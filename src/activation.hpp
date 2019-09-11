#pragma once

#include "config.h"

#include "types.hpp"

#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Association/Definitions/server.hpp>
#include <xyz/openbmc_project/Software/Activation/server.hpp>
#include <xyz/openbmc_project/Software/ActivationBlocksTransition/server.hpp>
#include <xyz/openbmc_project/Software/ExtendedVersion/server.hpp>

namespace phosphor
{
namespace software
{
namespace updater
{

namespace sdbusRule = sdbusplus::bus::match::rules;

using ActivationBlocksTransitionInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Software::server::
        ActivationBlocksTransition>;

/** @class ActivationBlocksTransition
 *  @brief OpenBMC ActivationBlocksTransition implementation.
 *  @details A concrete implementation for
 *  xyz.openbmc_project.Software.ActivationBlocksTransition DBus API.
 */
class ActivationBlocksTransition : public ActivationBlocksTransitionInherit
{
  public:
    /** @brief Constructs ActivationBlocksTransition.
     *
     * @param[in] bus    - The Dbus bus object
     * @param[in] path   - The Dbus object path
     */
    ActivationBlocksTransition(sdbusplus::bus::bus& bus,
                               const std::string& path) :
        ActivationBlocksTransitionInherit(bus, path.c_str(), true),
        bus(bus), path(path)
    {
        std::vector<std::string> interfaces({interface});
        bus.emit_interfaces_added(path.c_str(), interfaces);
    }

    ~ActivationBlocksTransition()
    {
        std::vector<std::string> interfaces({interface});
        bus.emit_interfaces_removed(path.c_str(), interfaces);
    }

  private:
    // TODO Remove once openbmc/openbmc#1975 is resolved
    static constexpr auto interface =
        "xyz.openbmc_project.Software.ActivationBlocksTransition";
    sdbusplus::bus::bus& bus;
    std::string path;
};

using ActivationInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Software::server::ExtendedVersion,
    sdbusplus::xyz::openbmc_project::Software::server::Activation,
    sdbusplus::xyz::openbmc_project::Association::server::Definitions>;

/** @class Activation
 *  @brief OpenBMC activation software management implementation.
 *  @details A concrete implementation for
 *  xyz.openbmc_project.Software.Activation DBus API.
 */
class Activation : public ActivationInherit
{
  public:
    using Status = Activations;
    /** @brief Constructs Activation Software Manager
     *
     * @param[in] bus    - The Dbus bus object
     * @param[in] path   - The Dbus object path
     * @param[in] versionId  - The software version id
     * @param[in] extVersion - The extended version
     * @param[in] activationStatus - The status of Activation
     * @param[in] assocs - Association objects
     */
    Activation(sdbusplus::bus::bus& bus, const std::string& path,
               const std::string& versionId, const std::string& extVersion,
               sdbusplus::xyz::openbmc_project::Software::server::Activation::
                   Activations activationStatus,
               const AssociationList& assocs) :
        ActivationInherit(bus, path.c_str(), true),
        versionId(versionId), bus(bus), path(path),
        systemdSignals(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("JobRemoved") +
                sdbusRule::path("/org/freedesktop/systemd1") +
                sdbusRule::interface("org.freedesktop.systemd1.Manager"),
            std::bind(&Activation::unitStateChange, this,
                      std::placeholders::_1))
    {
        // Set Properties.
        extendedVersion(extVersion);
        activation(activationStatus);
        associations(assocs);

        // Emit deferred signal.
        emit_object_added();
    }

    /** @brief Overloaded Activation property setter function
     *
     * @param[in] value - One of Activation::Activations
     *
     * @return Success or exception thrown
     */
    Activations activation(Activations value) override;

    /** @brief Activation */
    using ActivationInherit::activation;

    /** @brief Overloaded requestedActivation property setter function
     *
     * @param[in] value - One of Activation::RequestedActivations
     *
     * @return Success or exception thrown
     */
    RequestedActivations
        requestedActivation(RequestedActivations value) override;

    /** @brief Version id */
    std::string versionId;

  private:
    /** @brief Check if systemd state change is relevant to this object
     *
     * Instance specific interface to handle the detected systemd state
     * change
     *
     * @param[in]  msg       - Data associated with subscribed signal
     *
     */
    void unitStateChange(sdbusplus::message::message& msg);

    /**
     * @brief Delete the version from Image Manager and the
     *        untar image from image upload dir.
     */
    void deleteImageManagerObject();

    /** @brief Start PSU update */
    void startActivation();

    /** @brief Finish PSU update */
    void finishActivation();

    /** @brief Persistent sdbusplus DBus bus connection */
    sdbusplus::bus::bus& bus;

    /** @brief Persistent DBus object path */
    std::string path;

    /** @brief Used to subscribe to dbus systemd signals */
    sdbusplus::bus::match_t systemdSignals;

    /** @brief The PSU update systemd unit */
    std::string psuUpdateUnit;

    /** @brief Persistent ActivationBlocksTransition dbus object */
    std::unique_ptr<ActivationBlocksTransition> activationBlocksTransition;
};

} // namespace updater
} // namespace software
} // namespace phosphor
