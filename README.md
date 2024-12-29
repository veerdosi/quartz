# Quartz

Quartz combines quantum-inspired algorithms with real-time market data to optimize portfolio allocation and automate trading decisions. The system integrates with IEX Cloud for market data and Interactive Brokers for trade execution.

## Key Features

- Quantum-inspired portfolio optimization
- Real-time market data processing
- Automated trade execution
- Risk management and monitoring
- Multi-asset support
- Paper trading capabilities

## System Requirements

- Linux or macOS
- C++17 compiler (GCC 7+ or Clang 6+)
- CMake 3.15+
- Boost 1.70+
- Lua 5.3
- yaml-cpp

## Dependencies Installation

### Ubuntu/Debian

```bash
sudo apt-get update && sudo apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    liblua5.3-dev \
    libyaml-cpp-dev
```

### macOS

```bash
brew install \
    boost \
    lua@5.3 \
    yaml-cpp
```

## API Requirements

1. IEX Cloud Account

   - Register at https://iexcloud.io
   - Obtain API tokens
   - Subscribe to required data feeds

2. Interactive Brokers Account
   - Set up account at https://www.interactivebrokers.com
   - Install TWS or IB Gateway
   - Enable API connections
   - Configure API credentials

## Building

```bash
git clone https://github.com/yourusername/quartz.git
cd quartz
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Configuration

1. Copy sample configuration:

```bash
cp config/config.yaml.example config/config.yaml
```

2. Update configuration:

```yaml
market:
  provider: "iex"
  api_key: "YOUR_IEX_KEY"
  websocket_url: "wss://ws-cloud.iexapis.com/stable"

trading:
  provider: "ibkr"
  host: "gateway.interactivebrokers.com"
  port: "4001"
  client_id: "QUARTZ_01"
```

## Running

1. Start Interactive Brokers TWS/Gateway
2. Run Quartz:

```bash
./Quartz config/config.yaml
```

## Development

### Adding New Features

1. Strategy Implementation

   - Use `src/QuantumOptimizer.hpp` for optimization logic
   - Implement new strategies in Lua scripts
   - Add market data handlers in `MarketIntegration.hpp`

2. Risk Management
   - Configure risk parameters in `config.yaml`
   - Implement additional risk checks in core components

## Monitoring

The system provides multiple monitoring options:

- Real-time logging
- Email alerts
- Slack notifications
- Performance metrics
- Position tracking

## Production Deployment

1. Basic Setup:

   - Set `paper_trading: false` in config
   - Configure proper API credentials
   - Set up monitoring alerts

2. Advanced Configuration:
   - Implement redundant data sources
   - Configure database logging
   - Set up proper error handling
   - Enable SSL/TLS for connections

## License

MIT License - See LICENSE file for details

## Disclaimer

This software is for educational and research purposes only. Always perform thorough testing before using any trading system with real money. Past performance does not guarantee future results.
