# pragma once

#include <vector>
#include <complex>
#include <random>
#include <cmath>

namespace quantum_allocation {

class QuantumOptimizer {
public:
    struct OptimizationParameters {
        double risk_aversion;
        double temperature;
        int num_iterations;
        double learning_rate;
    };

    class QuantumCircuit {
    public:
        QuantumCircuit(size_t num_qubits) : num_qubits_(num_qubits) {
            state_.resize(1 << num_qubits, 0.0);
            state_[0] = 1.0;  // Initialize to |0...0⟩
        }

        void hadamard(size_t qubit) {
            const std::complex<double> h = 1.0 / std::sqrt(2.0);
            for (size_t i = 0; i < state_.size(); i++) {
                if (i & (1 << qubit)) {
                    std::complex<double> temp = state_[i];
                    state_[i] = h * (state_[i ^ (1 << qubit)] - temp);
                    state_[i ^ (1 << qubit)] = h * (temp + state_[i ^ (1 << qubit)]);
                }
            }
        }

        void phase(size_t qubit, double angle) {
            std::complex<double> phase = std::polar(1.0, angle);
            for (size_t i = 0; i < state_.size(); i++) {
                if (i & (1 << qubit)) {
                    state_[i] *= phase;
                }
            }
        }

        void controlled_phase(size_t control, size_t target, double angle) {
            std::complex<double> phase = std::polar(1.0, angle);
            for (size_t i = 0; i < state_.size(); i++) {
                if ((i & (1 << control)) && (i & (1 << target))) {
                    state_[i] *= phase;
                }
            }
        }

        std::vector<double> measure() {
            std::vector<double> probabilities(num_qubits_);
            for (size_t i = 0; i < state_.size(); i++) {
                double prob = std::norm(state_[i]);
                for (size_t q = 0; q < num_qubits_; q++) {
                    if (i & (1 << q)) {
                        probabilities[q] += prob;
                    }
                }
            }
            return probabilities;
        }

    private:
        size_t num_qubits_;
        std::vector<std::complex<double>> state_;
    };

    QuantumOptimizer(size_t num_assets, const OptimizationParameters& params)
        : num_assets_(num_assets), params_(params), circuit_(num_assets) {
        initializeCircuit();
    }

    std::vector<double> optimize(const std::vector<double>& returns,
                               const std::vector<std::vector<double>>& covariance) {
        for (int iter = 0; iter < params_.num_iterations; iter++) {
            // Apply quantum operations based on market data
            applyMarketData(returns, covariance);
            
            // Apply quantum annealing-inspired operations
            applyQuantumAnnealing(static_cast<double>(iter) / params_.num_iterations);
        }

        return circuit_.measure();
    }

private:
    void initializeCircuit() {
        // Apply Hadamard gates to create superposition
        for (size_t i = 0; i < num_assets_; i++) {
            circuit_.hadamard(i);
        }
    }

    void applyMarketData(const std::vector<double>& returns,
                        const std::vector<std::vector<double>>& covariance) {
        // Apply phase rotations based on expected returns
        for (size_t i = 0; i < num_assets_; i++) {
            double angle = returns[i] * params_.learning_rate;
            circuit_.phase(i, angle);
        }

        // Apply controlled phase rotations based on covariances
        for (size_t i = 0; i < num_assets_; i++) {
            for (size_t j = i + 1; j < num_assets_; j++) {
                double angle = covariance[i][j] * params_.risk_aversion;
                circuit_.controlled_phase(i, j, angle);
            }
        }
    }

    void applyQuantumAnnealing(double progress) {
        double temperature = params_.temperature * (1.0 - progress);
        for (size_t i = 0; i < num_assets_; i++) {
            circuit_.phase(i, temperature * std::sin(M_PI * progress));
        }
    }

    size_t num_assets_;
    OptimizationParameters params_;
    QuantumCircuit circuit_;
};

} // namespace quantum_allocation