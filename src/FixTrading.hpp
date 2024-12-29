// FixTrading.hpp
#pragma once

#include <quickfix/Application.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/Values.h>
#include <quickfix/SocketInitiator.h>
#include <quickfix/Session.h>
#include <memory>
#include <string>

namespace quantum_allocation {

class FixTrading : public FIX::Application, public FIX::MessageCracker {
public:
    FixTrading() {
        // Initialize FIX settings
        settings_.setString(FIX::BEGINSTRING, "FIX.4.4");
        settings_.setString(FIX::SENDERCOMPID, "QUANTUM_ALLOC");
        settings_.setString(FIX::TARGETCOMPID, "BROKER");
        settings_.setString(FIX::CONNECTION_TYPE, "initiator");
        
        initiator_ = std::make_unique<FIX::SocketInitiator>(*this, storeFactory_, settings_);
    }
    
    void start() {
        initiator_->start();
    }
    
    void stop() {
        initiator_->stop();
    }
    
    // Send a new order
    void sendOrder(const std::string& symbol, char side, double quantity, double price) {
        FIX44::NewOrderSingle message;
        message.setField(FIX::ClOrdID(getNextOrderID()));
        message.setField(FIX::Symbol(symbol));
        message.setField(FIX::Side(side));
        message.setField(FIX::OrderQty(quantity));
        message.setField(FIX::Price(price));
        message.setField(FIX::OrdType(FIX::OrdType_LIMIT));
        message.setField(FIX::TimeInForce(FIX::TimeInForce_DAY));
        
        FIX::Session::sendToTarget(message);
    }
    
private:
    // FIX::Application interface implementation
    void onCreate(const FIX::SessionID&) override {}
    void onLogon(const FIX::SessionID&) override {}
    void onLogout(const FIX::SessionID&) override {}
    void toAdmin(FIX::Message&, const FIX::SessionID&) override {}
    void toApp(FIX::Message&, const FIX::SessionID&) throw(FIX::DoNotSend) override {}
    void fromAdmin(const FIX::Message&, const FIX::SessionID&) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override {}
    
    void fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) throw(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override {
        crack(message, sessionID);
    }
    
    // Message handlers
    void onMessage(const FIX44::ExecutionReport& message, const FIX::SessionID&) {
        FIX::ExecType execType;
        message.getField(execType);
        
        if (execType == FIX::ExecType_FILL) {
            FIX::Symbol symbol;
            FIX::Side side;
            FIX::LastQty lastQty;
            FIX::LastPx lastPx;
            
            message.getField(symbol);
            message.getField(side);
            message.getField(lastQty);
            message.getField(lastPx);
            
            // Handle the fill
            handleFill(symbol, side, lastQty, lastPx);
        }
    }
    
    void handleFill(const FIX::Symbol& symbol, const FIX::Side& side, 
                   const FIX::LastQty& qty, const FIX::LastPx& price) {
        // Update portfolio with execution details
        // Implement fill handling logic here
    }
    
    std::string getNextOrderID() {
        return "ORD" + std::to_string(++orderID_);
    }
    
    FIX::SessionSettings settings_;
    FIX::FileStoreFactory storeFactory_;
    std::unique_ptr<FIX::SocketInitiator> initiator_;
    std::atomic<int> orderID_{0};
};

} // namespace quantum_allocation
