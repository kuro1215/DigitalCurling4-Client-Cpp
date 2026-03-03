// Copyright (c) 2022-2026 UEC Takeshi Ito Laboratory
// SPDX-License-Identifier: MIT

#pragma once

#include <digitalcurling/simulators/plugin_simulator.hpp>
#include "digitalcurling/client/i_factory_creator.hpp"
#include "digitalcurling/client/i_thinking_engine.hpp"

namespace digitalcurling::client {

class RulebasedEngine :
    public IStandardThinkingEngine, public IMixedThinkingEngine , public IMixedDoublesThinkingEngine
{
public:
    RulebasedEngine() : IStandardThinkingEngine(), IMixedThinkingEngine(), IMixedDoublesThinkingEngine() {}

    virtual inline std::string GetName() const override {
        return "rulebased";
    }

    virtual std::vector<std::uint8_t> OnInit(
        GameRule const& game_rule,
        GameSetting const& game_setting,
        std::unique_ptr<simulators::ISimulatorFactory> simulator,
        std::vector<std::unique_ptr<players::IPlayerFactory>> const& players
    ) override;

    virtual void OnGameStart(
        Team const& team,
        std::vector<std::pair<digitalcurling::GameState, std::optional<moves::Shot>>> states
    ) override;
    virtual void OnNextEnd(GameState const& game_state) override;
    PositionedStoneOptions OnDecidePositionedStone(GameState const& game_state) override;

    virtual moves::Move OnMyTurn(
        std::unique_ptr<players::IPlayerFactory> const& player_factory,
        GameState const& game_state,
        std::optional<moves::Shot> const& last_shot
    ) override;

    virtual void OnOpponentTurn(
        GameState const& game_state,
        std::optional<moves::Shot> const& last_shot
    ) override;

    virtual void OnGameOver(GameState const& game_state) override;

private:
    Team team_;
    GameRule game_rule_;
    GameSetting game_setting_;
    std::unique_ptr<simulators::InvertiblePluginSimulator> simulator_;
};

} // namespace digitalcurling::client
