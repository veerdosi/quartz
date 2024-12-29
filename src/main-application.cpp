#include "QuantumOptimizer.hpp"
#include "MarketIntegration.hpp"
#include "FixTrading.hpp"
#include "LuaInterface.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <yaml-cpp/yaml.h>

namespace po = boost::program_options;

class QuantumAllocationSystem {
public:
    struct Config {
        // Market data settings
        std::string market_host;
        std::string market_port;
        std::vector<std::string> symbols;
        
        // Optimization parameters
        double risk_aversion;
        double initial_temperature;
        int num_iterations;
        double learning_rate;
        
        // Trading parameters
        int rebalance_interval;
        double min_trade_size;
        double max_position_size;
        
        // Risk parameters
        double var_confidence;
        double max_drawdown_limit;
    };

    QuantumAllocationSystem(const std::string& config_path)
        : running_(true),
          ioc_(),
          work_guard_(boost::asio::make_work_guard(ioc_)),
          market_data_(ioc_),
          fix_trading_(),
          lua_interface_() {
        loadConfig(config_path);
        initializeComponents();
    }

    void run() {
        // Start IO context in separate thread
        std::thread io_thread([this]() {
            try {
                ioc_.run();
            } catch (const std::exception& e) {
                std::cerr << "IO context error: " << e.what() << std::endl;
                stop();
            }
        });

        try {
            // Initialize market data connection
            market_data_.connect(config_.market_host, config_.market_port);
            for (const auto& symbol : config_.symbols) {
                market_data_.subscribe(symbol);
            }

            // Start FIX trading
            fix_trading_.start();

            // Initialize quantum optimizer
            QuantumOptimizer::OptimizationParameters opt_params{
                config_.risk_aversion,
                config_.initial_temperature,
                config_.num_iterations,
                config_.learning_rate
            };
            
            QuantumOptimizer optimizer(config_.symbols.size(), opt_params);
            RiskManager risk_manager(config_.var_confidence);

            std::cout << "Starting main optimization loop..." << std::endl;
            mainLoop(optimizer, risk_manager);

        } catch (const std::exception& e) {
            std::cerr << "Fatal error: " << e.what() << std::endl;
            stop();
        }

        io_thread.join();
    }

    void stop() {
        running_ = false;
        fix_trading_.stop();
        work_guard_.reset();
        ioc_.stop();
    }

private:
    void loadConfig(const std::string& config_path) {
        try {
            YAML::Node yaml = YAML::LoadFile(config_path);
            
            // Load market settings
            auto market = yaml["market"];
            config_.market_host = market["host"].as<std::string>();
            config_.market_port = market["port"].as<std::string>();
            config_.symbols = market["symbols"].as<std::vector<std::string>>();

            // Load optimization settings
            auto optimization = yaml["optimization"];
            config_.risk_aversion = optimization["risk_aversion"].as<double>();
            config_.initial_temperature = optimization["initial_temperature"].as<double>();
            config_.num_iterations = optimization["num_iterations"].as<int>();
            config_.learning_rate = optimization["learning_rate"].as<double>();

            // Load trading settings
            auto trading = yaml["trading"];
            config_.rebalance_interval = trading["rebalance_interval"].as<int>();
            config_.min_trade_size = trading["min_trade_size"].as<double>();
            config_.max_position_size = trading["max_position_size"].as<double>();

            // Load risk settings
            auto risk = yaml["risk"];
            config_.var_confidence = risk["var_confidence"].as<double>();
            config_.max_drawdown_limit = risk["max_drawdown_limit"].as<double>();

        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to load config: " + std::string(e.what()));
        }
    }

    void initializeComponents() {
        // Initialize LUA interface
        lua_interface_.setPortfolio(&fix_trading_);
        
        // Load strategy scripts
        std::string strategy_path = "strategies/main.lua";
        if (!lua_interface_.executeScript(strategy_path)) {
            throw std::runtime_error("Failed to load strategy script");
        }
    }

    void mainLoop(QuantumOptimizer& optimizer, RiskManager& risk_manager) {
        while (running_) {
            try {
                // Collect market data
                auto market_data = collectMarketData();
                
                // Run optimization
                auto weights = optimizer.optimize(
                    market_data.returns,
                    market_data.covariance
                );

                // Calculate risk metrics
                auto risk_metrics = risk_manager.calculateRiskMetrics(
                    market_data.returns,
                    weights
                );

                // Check risk limits
                if (risk_metrics.max_drawdown > config_.max_drawdown_limit) {
                    std::cout << "Warning: Max drawdown limit exceeded" << std::endl;
                    reduceRisk(weights);
                }

                // Execute strategy adjustments
                executeLuaStrategy(weights, risk_metrics);

                // Execute trades
                executeTrades(weights, market_data);

                // Log state
                logState(weights, risk_metrics, market_data);

                std::this_thread::sleep_for(
                    std::chrono::seconds(config_.rebalance_interval)
                );

            } catch (const std::exception& e) {
                std::cerr << "Error in main loop: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }

    struct MarketData {
        std::vector<double> returns;
        std::vector<std::vector<double>> covariance;
        std::vector<double> current_prices;
    };

    MarketData collectMarketData() {
        MarketData data;
        for (const auto& symbol : config_.symbols) {
            auto market_update = market_data_.getLatestData(symbol);
            data.current_prices.push_back(market_update.price);
            // Calculate returns and covariance here
        }
        return data;
    }

    void executeTrades(const std::vector<double>& target_weights, 
                      const MarketData& market_data) {
        for (size_t i = 0; i < config_.symbols.size(); ++i) {
            double current_weight = getCurrentWeight(config_.symbols[i]);
            double weight_diff = target_weights[i] - current_weight;
            
            if (std::abs(weight_diff) > config_.min_trade_size) {
                double quantity = calculateQuantity(
                    weight_diff, 
                    market_data.current_prices[i]
                );
                
                char side = (weight_diff > 0) ? 'B' : 'S';
                fix_trading_.sendOrder(
                    config_.symbols[i],
                    side,
                    std::abs(quantity),
                    market_data.current_prices[i]
                );
            }
        }
    }

    void logState(const std::vector<double>& weights,
                 const RiskManager::RiskMetrics& risk_metrics,
                 const MarketData& market_data) {
        // Implement logging
    }

    std::atomic<bool> running_;
    boost::asio::io_context ioc_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    MarketDataFeed market_data_;
    FixTrading fix_trading_;
    LuaInterface lua_interface_;
    Config config_;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <config_path>" << std::endl;
            return 1;
        }

        QuantumAllocationSystem system(argv[1]);
        
        // Handle signals
        boost::asio::signal_set signals(system.getIoContext(), SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code& ec, int signal) {
            std::cout << "Shutting down..." << std::endl;
            system.stop();
        });

        system.run();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
