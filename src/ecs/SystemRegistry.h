#pragma once

#include <functional>
#include <string>
#include <vector>

namespace rigkit {

/** @brief When a registered system runs in the host loop. */
enum class SystemPhase { Update, Draw };

/**
 * @brief Stored system call. The ECS it belongs to is already bound.
 * @details `MEcs::registerSystem` wraps whatever the pack passed, so a system
 * never receives the manager it was registered on.
 */
using SystemFn = std::function<void(float /*dt*/)>;

/** @brief Named fulfillment callback owned by a pack. */
struct SystemEntry {
	std::string name;
	SystemPhase phase = SystemPhase::Update;
	SystemFn fn;
};

} // namespace rigkit
