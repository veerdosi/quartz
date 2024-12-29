#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <nlohmann/json.hpp>
#include <queue>
#include <mutex>

namespace quantum_allocation {

using json = nlohmann::json;

class MarketDataFeed {
public:
    struct MarketData {
        std::string symbol;
        double price;
        double volume;
        double bid;
        double ask;
        std::chrono::system_clock::time_point timestamp;
    };

    MarketDataFeed(boost::asio::io_context& ioc)
        : ioc_(ioc), ws_(std::make_unique<boost::beast::websocket::stream<boost::beast::tcp_stream>>(ioc)) {
    }

    void connect(const std::string& host, const std::string& port) {
        boost::asio::ip::tcp::resolver resolver(ioc_);
        auto const results = resolver.resolve(host, port);

        boost::beast::get_lowest_layer(*ws_).connect(results);
        ws_->handshake(host, "/ws");

        asyncRead();
    }

    void subscribe(const std::string& symbol) {
        json subscription = {
            {"type", "subscribe"},
            {"symbol", symbol}
        };
        ws_->write(boost::asio::buffer(subscription.dump()));
    }

    MarketData getLatestData(const std::string& symbol) {
        std::lock_guard<std::mutex> lock(mutex_);
        return latest_data_[symbol];
    }

private:
    void asyncRead() {
        ws_->async_read(
            buffer_,
            [this](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    processMessage(boost::beast::buffers_to_string(buffer_.data()));
                    buffer_.consume(buffer_.size());
                    asyncRead();
                }
            });
    }

    void processMessage(const std::string& message) {
        try {
            json j = json::parse(message);
            MarketData data{
                j["symbol"].get<std::string>(),
                j["price"].get<double>(),
                j["volume"].get<double>(),
                j["bid"].get<double>(),
                j["ask"].get<double>(),
                std::chrono::system_clock::now()
            };

            std::lock_guard<std::mutex> lock(mutex_);
            latest_data_[data.symbol] = data;
        } catch (const std::exception& e) {
            std::cerr << "Error processing market data: " << e.what() << std::endl;
        }
    }

    boost::asio::io_context& ioc_;
    std::unique_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>> ws_;
    boost::beast::flat_buffer buffer_;
    std::mutex mutex_;
    std::map<std::string, MarketData> latest_data_;
};

class RiskManager {
public:
    struct RiskMetrics {
        double var;           // Value at Risk
        double cvar;          // Conditional Value at Risk
        double sharpe_ratio;
        double max_drawdown;
    };

    RiskManager(double confidence_level = 0.95, int var_window = 252)
        : confidence_level_(confidence_level), var_window_(var_window) {}

    RiskMetrics calculateRiskMetrics(const std::vector<double>& returns,
                                   const std::vector<double>& weights) {
        RiskMetrics metrics;
        
        // Calculate portfolio returns
        std::vector<double> portfolio_returns(returns.size(), 0.0);
        for (size_t i = 0; i < returns.size(); ++i) {
            for (size_t j = 0; j < weights.size(); ++j) {
                portfolio_returns[i] += returns[i] * weights[j];
            }
        }

        // Sort returns for VaR calculation
        std::vector<double> sorted_returns = portfolio_returns;
        std::sort(sorted_returns.begin(), sorted_returns.end());

        // Calculate VaR
        size_t var_index = static_cast<size_t>((1.0 - confidence_level_) * sorted_returns.size());
        metrics.var = -sorted_returns[var_index];

        // Calculate CVaR
        double cvar_sum = 0.0;
        size_t count = 0;
        for (size_t i = 0; i < var_index; ++i) {
            cvar_sum += sorted_returns[i];
            ++count;
        }
        metrics.cvar = -cvar_sum / count;

        // Calculate Sharpe Ratio
        double mean_return = std::accumulate(portfolio_returns.begin(), 
                                           portfolio_returns.end(), 0.0) / portfolio_returns.size();
        double variance = 0.0;
        for (double r : portfolio_returns) {
            variance += (r - mean_return) * (r - mean_return);
        }
        variance /= (portfolio_returns.size() - 1);
        double std_dev = std::sqrt(variance);
        metrics.sharpe_ratio = mean_return / std_dev;

        // Calculate Maximum Drawdown
        double peak = portfolio_returns[0];
        double max_drawdown = 0.0;
        for (double r : portfolio_returns) {
            if (r > peak) peak = r;
            double drawdown = (peak - r) / peak;
            if (drawdown > max_drawdown) max_drawdown = drawdown;
        }
        metrics.max_drawdown = max_drawdown;

        return metrics;
    }

private:
    double confidence_level_;
    int var_window_;
};

} // namespace quantum_allocation