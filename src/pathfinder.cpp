#include <set>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <functional>
#include <atomic>
#include <Level.hpp>
#include <gdr/gdr.hpp>
#include "pathfinder.hpp"
 
// Simple replay wrapper
class Replay2 : public gdr::Replay<Replay2, gdr::Input<"">> {
public:
    Replay2() : Replay("Pathfinder", 1) {}
};
 
// Level wrapper with press state
struct Level2 : public Level {
    bool press = false;
    float highestY = 0;
    using Level::Level;
 
    Level2(std::string const& lvlString) : Level(lvlString) {
        for (auto& i : sections)
            for (auto& j : i)
                highestY = std::max(highestY, j->pos.y);
    }
};
 
// ---- Compact state key for deduplication ----
// Quantize to avoid floating-point noise while preserving meaningful differences
struct StateKey {
    int x, y, vx, vy;
    bool press;
 
    bool operator==(const StateKey& o) const {
        return x == o.x && y == o.y && vx == o.vx && vy == o.vy && press == o.press;
    }
};
 
struct StateKeyHash {
    size_t operator()(const StateKey& k) const noexcept {
        // FNV-1a-style mix for low collision rate
        size_t h = 2166136261u;
        auto mix = [&](int v) {
            h ^= (size_t)(uint32_t)v;
            h *= 16777619u;
        };
        mix(k.x); mix(k.y); mix(k.vx); mix(k.vy); mix((int)k.press);
        return h;
    }
};
 
static StateKey makeKey(const Level2& lvl, bool press) {
    auto st = lvl.latestState();
    // Quantize at 1/10 unit resolution — fine enough to distinguish meaningful states
    return {
        (int)(st.pos.x * 10.f),
        (int)(st.pos.y * 10.f),
        (int)(st.vel.x * 10.f),
        (int)(st.vel.y * 10.f),
        press
    };
}
 
// ---- Solver state ----
struct SolverState {
    Level2           lvl;
    bool             press;
    int              frame;
    std::vector<uint16_t> inputSequence;
    float            score;  // heuristic for priority ordering
 
    SolverState(Level2 const& l, bool p, int f,
                std::vector<uint16_t> seq, float sc)
        : lvl(l), press(p), frame(f),
          inputSequence(std::move(seq)), score(sc) {}
 
    // Higher score = explored first
    bool operator<(const SolverState& o) const { return score < o.score; }
};
 
// ---- Scoring heuristic ----
// Rewards forward progress heavily; gives a small bonus for height
// (many GD levels require going up before going forward).
// Penalises very long input sequences to prefer efficient solutions.
static float computeScore(const Level2& lvl, int frame) {
    auto st = lvl.latestState();
    float xProgress = st.pos.x / lvl.length;              // 0..1
    float yBonus    = st.pos.y / (lvl.highestY + 1.f);    // small tiebreaker
    float efficiency = 1.f - std::min(frame / 7200.f, 1.f); // prefer shorter runs
    return xProgress * 10.f + yBonus * 0.5f + efficiency * 0.1f;
}
 
// ---- Beam-width schedule ----
// Start narrow for speed; widen automatically if we get stuck.
static size_t adaptBeamWidth(float prevBest, float curBest, size_t curWidth) {
    if (curBest - prevBest < 0.001f) {
        // Stuck: double beam width up to a cap
        return std::min(curWidth * 2, (size_t)8000);
    }
    // Progressing: shrink back toward baseline for speed
    return std::max(curWidth / 2, (size_t)1000);
}
 
// ---- Main pathfinder ----
std::vector<uint8_t> pathfind(std::string const& lvlString,
                               std::atomic_bool& stop,
                               std::function<void(double)> callback) {
    Level2 lvl(lvlString);
 
    // Priority queue: best-first (highest score pops first)
    using PQ = std::priority_queue<SolverState>;
    PQ beam;
    std::unordered_set<StateKey, StateKeyHash> visited;
 
    // Seed
    float initScore = computeScore(lvl, 0);
    SolverState init(lvl, false, 0, {}, initScore);
    visited.insert(makeKey(lvl, false));
    beam.push(std::move(init));
 
    SolverState bestState = beam.top();
    float       prevBestX = 0.f;
    size_t      beamWidth = 1500;
    int         stallTurns = 0;
 
    while (!beam.empty() && !stop) {
        // ---- Drain current beam into candidate pool ----
        std::vector<SolverState> nextStates;
        nextStates.reserve(beamWidth * 4);
 
        while (!beam.empty()) {
            SolverState s = std::move(const_cast<SolverState&>(beam.top()));
            beam.pop();
 
            auto curX = s.lvl.latestState().pos.x;
 
            // Victory condition
            if (curX >= s.lvl.length) {
                bestState = std::move(s);
                goto finish;
            }
 
            // Expand: try press and release
            for (bool action : {true, false}) {
                Level2 lvlCopy = s.lvl;
                lvlCopy.runFrame(action);
 
                if (lvlCopy.gameStates.back().dead)
                    continue;
 
                StateKey key = makeKey(lvlCopy, action);
                if (visited.count(key))
                    continue;
                visited.insert(key);
 
                // Build input sequence — only record toggle points
                std::vector<uint16_t> seq = s.inputSequence;
                if (action != s.press)
                    seq.push_back((uint16_t)s.frame);
 
                float sc = computeScore(lvlCopy, s.frame + 1);
                SolverState next(lvlCopy, action, s.frame + 1, std::move(seq), sc);
 
                // Track global best
                if (lvlCopy.latestState().pos.x > bestState.lvl.latestState().pos.x)
                    bestState = next;  // copy intentional
 
                nextStates.push_back(std::move(next));
            }
        }
 
        if (nextStates.empty()) break;
 
        // ---- Select top-beamWidth states by score ----
        // partial_sort is faster than full sort when beamWidth << nextStates.size()
        size_t keep = std::min(nextStates.size(), beamWidth);
        std::partial_sort(
            nextStates.begin(),
            nextStates.begin() + (std::ptrdiff_t)keep,
            nextStates.end(),
            [](const SolverState& a, const SolverState& b) {
                return a.score > b.score;  // descending
            }
        );
 
        float curBestX = bestState.lvl.latestState().pos.x;
 
        // Push survivors into the next beam
        for (size_t i = 0; i < keep; ++i)
            beam.push(std::move(nextStates[i]));
 
        // ---- Adaptive beam width ----
        beamWidth = adaptBeamWidth(prevBestX, curBestX, beamWidth);
        if (curBestX <= prevBestX + 0.001f)
            ++stallTurns;
        else
            stallTurns = 0;
        prevBestX = curBestX;
 
        // ---- Progress callback ----
        if (callback) {
            double pct = std::min(
                (double)(curBestX / bestState.lvl.length) * 100.0,
                100.0
            );
            callback(pct);
        }
 
        // Safety valve: if we've been completely stuck for many turns, give up
        if (stallTurns > 120) break;
    }
 
finish:
    Replay2 output;
    bool currentPress = false;
    for (uint16_t f : bestState.inputSequence) {
        currentPress = !currentPress;
        // gdr::Input(frame, player, releasedBefore, pressed)
        output.inputs.push_back(gdr::Input(f, 1, !currentPress, currentPress));
    }
 
    return output.exportData().unwrapOr({});
}
 
