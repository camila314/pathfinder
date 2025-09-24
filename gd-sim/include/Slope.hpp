#pragma once
#include <Block.hpp>

struct Slope : public Block {
    int orientation;
	
    Slope(Vec2D size, std::unordered_map<int, std::string>&& fields);
    void collide(Player&) const override;
    bool touching(Player const&) const override;

    /**
     * On certain slopes, the player can still be "on" them without physically
     * touching them. This function is called by Player separately and accounts
     * for this.
     */
    void calc(Player& p) const;

    /// Orientation of the slope relative to the player's gravity.
    int gravOrient(Player const& p) const;

    double angle() const;
    bool isFacingUp() const;

    /**
     * The Y position that a player should be snapped to if on the slope.
     * Also used for collision detection
     */
    virtual double expectedY(Player const& p) const;
};


struct SlopeHazard : public Slope {
    using Slope::Slope;
    void collide(Player&) const override;
    bool touching(Player const&) const override;
    double expectedY(Player const& p) const override;
};