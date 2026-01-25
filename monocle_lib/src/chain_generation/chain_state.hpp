#pragma once

#include "../tp_ring_queue.hpp"

namespace mon {

struct TeleportChainInternalState {

    using queue_entry = int;
    using queue_type = RingQueue<queue_entry, 8>;

    using portal_type = queue_entry;
    static constexpr queue_entry FUNC_RECHECK_COLLISION = 0;
    static constexpr portal_type FUNC_TP_BLUE = 1;
    static constexpr portal_type FUNC_TP_ORANGE = 2;
    static constexpr portal_type PORTAL_NONE = 0;

    /*
    * CPortalTouchScope::m_CallQueue. Holds values representing:
    * - FUNC_TP_BLUE: a blue teleport
    * - FUNC_TP_ORANGE: an orange teleport
    * - FUNC_RECHECK_COLLISION: RecheckEntityCollision
    * - negative values: a null
    */
    queue_type tp_queue;
    // total number of nulls queued so far, only for debugging
    queue_entry n_queued_nulls;
    int touch_scope_depth; // CPortalTouchScope::m_nDepth, not fully implemented

    portal_type owning_portal;
};

} // namespace mon
