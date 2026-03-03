// Copyright (c) 2022-2026 UEC Takeshi Ito Laboratory
// SPDX-License-Identifier: MIT

#include "digitalcurling/client/client_helpers.hpp"
#include "rulebased.hpp"

namespace digitalcurling::client {

// --- RulebasedEngine ---
std::vector<std::uint8_t> RulebasedEngine::OnInit(
    GameRule const& game_rule,
    GameSetting const& game_setting,
    std::unique_ptr<simulators::ISimulatorFactory> simulator,
    std::vector<std::unique_ptr<players::IPlayerFactory>> const& players
) {
    auto sim = simulator->CreateSimulator();
    auto inv_sim = dynamic_cast<simulators::InvertiblePluginSimulator*>(sim.get());
    if (inv_sim == nullptr) {
        throw std::runtime_error("Simulator is not invertible simulator.");
    }

    sim.release();
    simulator_ = std::move(std::unique_ptr<simulators::InvertiblePluginSimulator>(inv_sim));

    game_rule_ = game_rule;
    game_setting_ = game_setting;

    if (game_rule_.type == GameRuleType::kMixedDoubles) {
        return {0, 1};
    } else {
        return {0, 1, 2, 3};
    }
}

void RulebasedEngine::OnGameStart(
    Team const& team,
    std::vector<std::pair<GameState, std::optional<moves::Shot>>> states
) {
    team_ = team;
}
void RulebasedEngine::OnNextEnd(GameState const& game_state) {
    // Nothing to do
}

IMixedDoublesThinkingEngine::PositionedStoneOptions RulebasedEngine::OnDecidePositionedStone(GameState const& game_state) {
    return PositionedStoneOptions::kCenterHouse;
}

moves::Move RulebasedEngine::OnMyTurn(
    std::unique_ptr<players::IPlayerFactory> const& player_factory,
    GameState const& game_state,
    std::optional<moves::Shot> const& last_shot
) {
    auto sorted = game_state.stones.GetSortedIndex();
    if (sorted.size() > 0) {
        auto const& no1_stone = game_state.stones[sorted[0]].value();

        if (no1_stone.IsInHouse()) {
            if (sorted[0].team != team_) {
                // No. 1 ストーンが敵チームのものならば，
                // No. 1 ストーンをテイクアウトするショットを行う

                std::array<moves::Shot, 2> const candidate_shots{{
                    simulator_->CalculateShot(no1_stone.position, 3.f, -1.57f),
                    simulator_->CalculateShot(no1_stone.position, 3.f,  1.57f)
                }};

                auto current_player = player_factory->CreatePlayer();
                auto stones = ConvertToSimulatorStones(game_state.stones);
                auto stone_no = static_cast<std::uint8_t>(team_) * 8 + ((game_state.shot + 1) / 2);

                constexpr std::uint32_t kTrials = 50;
                std::array<std::uint32_t, candidate_shots.size()> success_counts{{}};

                for (std::uint32_t i = 0; i < kTrials; ++i) {
                    for (size_t j = 0; j < candidate_shots.size(); ++j) {
                        auto temp_shot = current_player->Play(candidate_shots[j]);
                        stones[stone_no] =
                            simulators::ISimulator::StoneState(
                                Vector2 { 0.f, 0.f }, 0.f, temp_shot.ToVector2(), temp_shot.angular_velocity
                            );
                        simulator_->SetStones(stones);

                        SimulateFull(simulator_.get(), game_setting_.sheet_width);
                        auto simulated_stones = GetStoneCoordinateFromSimulator(simulator_.get());
                        auto violated_rule = game_rule_.VerifyShot(game_state.end, team_, game_state.stones, simulated_stones);

                        if (!violated_rule.has_value()) {
                            auto res_stone0 = simulated_stones[sorted[0]];
                            if (res_stone0.has_value() && res_stone0.value().IsInHouse())
                                success_counts[j]++;
                        }
                    }
                }

                size_t best_shot_idx = 0;
                unsigned best_count = success_counts[0];
                for (size_t i = 1; i < success_counts.size(); ++i) {
                    if (success_counts[i] > best_count) {
                        best_count = success_counts[i];
                        best_shot_idx = i;
                    }
                }

                return candidate_shots[best_shot_idx];
            } else {
                // No. 1 ストーンが自チームのものならば
                // 2m手前にガードストーンを置く

                Vector2 target_pos = no1_stone.position;
                target_pos.y -= 2.f;

                float ang_vel = no1_stone.position.x < coordinate::kTee.x ? -1.57f : 1.57f;
                return simulator_->CalculateShot(target_pos, 0.f, ang_vel);
            }
        }
    }

    // No. 1 ストーンがハウス内に無いなら
    // ティーの位置にストーンを投げる
    return simulator_->CalculateShot(coordinate::kTee, 0.f, -1.57f);
}
void RulebasedEngine::OnOpponentTurn(GameState const& game_state,std::optional<moves::Shot> const& last_shot) {
    // Nothing to do
}

void RulebasedEngine::OnGameOver(GameState const& game_state) {
    // Nothing to do
}

} // namespace digitalcurling::client
