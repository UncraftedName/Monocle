#include "generate.hpp"
#include "ulp_diff.hpp"

#include <cmath>

namespace mon {

std::pair<Entity, PointToPortalPlaneUlpDist> ProjectEntityToPortalPlane(const Entity& ent, const Portal& portal)
{
    const VPlane& plane = portal.plane;

    uint32_t ax = 0;
    for (int i = 1; i < 3; i++)
        if (std::fabsf(plane.n[i]) > std::fabsf(plane.n[ax]))
            ax = i;

    Vector old_center = ent.GetCenter();
    Entity proj_ent = ent;
    float& new_ax_val = proj_ent.GetPosRef()[ax]; // the component we're nudging
    float old_ax_val = new_ax_val;
    /*
    * plane: ax+by+cz=d, center: <i,j,k>
    * If e.g. we're projecting along the x-axis, then: x = (d-by-cz)/a = old_x + (d - plane*center) / a.
    * We're trying to project the ent center onto the plane, but for e.g. the player we must adjust
    * the origin, so use the difference of centers to adjust PosRef.
    */
    float new_center_ax_val = (float)(old_center[ax] + (plane.d - plane.n.Dot(old_center)) / plane.n[ax]);
    new_ax_val += new_center_ax_val - old_center[ax];
    /*
    * Mathematically, the entity is on the plane now. But we want to make sure it's as close as
    * possible. Move along the plane normal until we're in front, then move back.
    */
    for (int same_as_plane_norm = 1; same_as_plane_norm >= 0; same_as_plane_norm--) {
        float nudge_towards = !!same_as_plane_norm == std::signbit(plane.n[ax]) ? -INFINITY : INFINITY;
        while (portal.ShouldTeleport(proj_ent, false) == !!same_as_plane_norm)
            new_ax_val = std::nextafterf(new_ax_val, nudge_towards);
    }

    uint32_t ulp_diff = ulp::UlpDiffF(new_ax_val, old_ax_val);
    PointToPortalPlaneUlpDist dist_info{
        .n_ulps = ulp_diff,
        .ax = ax,
        .pt_was_behind_portal = ulp_diff == 0 || std::signbit(new_ax_val - old_ax_val) == std::signbit(plane.n[ax]),
        .is_valid = true,
    };
    return {proj_ent, dist_info};
}

} // namespace mon
