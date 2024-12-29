// LuaInterface.hpp
#pragma once

#include "QuantumAllocation.hpp"
#include <lua.hpp>
#include <string>

namespace quantum_allocation {

class LuaInterface {
public:
    LuaInterface() {
        L = luaL_newstate();
        luaL_openlibs(L);
        registerFunctions();
    }
    
    ~LuaInterface() {
        lua_close(L);
    }
    
    bool executeScript(const std::string& script) {
        if (luaL_dostring(L, script.c_str()) != 0) {
            std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
            return false;
        }
        return true;
    }
    
    void setPortfolio(QuantumPortfolio* portfolio) {
        portfolio_ = portfolio;
    }

private:
    void registerFunctions() {
        // Register C++ functions to be called from Lua
        lua_pushcfunction(L, [](lua_State* L) -> int {
            auto* self = (LuaInterface*)lua_touserdata(L, lua_upvalueindex(1));
            const char* symbol = luaL_checkstring(L, 1);
            self->portfolio_->addAsset(symbol);
            return 0;
        });
        lua_setglobal(L, "addAsset");
        
        lua_pushcfunction(L, [](lua_State* L) -> int {
            auto* self = (LuaInterface*)lua_touserdata(L, lua_upvalueindex(1));
            const char* symbol = luaL_checkstring(L, 1);
            double price = luaL_checknumber(L, 2);
            self->portfolio_->updatePrice(symbol, price);
            return 0;
        });
        lua_setglobal(L, "updatePrice");
    }
    
    lua_State* L;
    QuantumPortfolio* portfolio_;
};

} // namespace quantum_allocation

// Example Lua script (strategy.lua)
/*
-- Dynamic strategy adjustment
function adjustRiskParameters()
    local volatility = getMarketVolatility()
    if volatility > 0.2 then
        -- Increase defensive allocations
        setRiskAversion(0.8)
    else
        -- More aggressive stance
        setRiskAversion(0.4)
    end
end

-- Custom asset allocation constraints
function setCustomConstraints()
    addConstraint("AAPL", 0.0, 0.3)  -- Max 30% in AAPL
    addConstraint("GOOGL", 0.0, 0.25) -- Max 25% in GOOGL
end

-- Execute strategy
adjustRiskParameters()
setCustomConstraints()
*/
