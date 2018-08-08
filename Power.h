#ifndef __POWER_H
#define __POWER_H

#include "Module.h"
#include <interfaces/IPower.h>

namespace WPEFramework {
namespace Plugin {

class Power : public PluginHost::IPlugin, public PluginHost::IWeb {
private:
    Power(const Power&) = delete;
    Power& operator=(const Power&) = delete;

    class Notification : 
        public RPC::IRemoteProcess::INotification,
        public PluginHost::VirtualInput::Notifier {

    private:
        Notification() = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;

    public:
        explicit Notification(Power* parent)
            : _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }
        ~Notification()
        {
        }

    public:
        virtual void Activated(RPC::IRemoteProcess*)
        {
        }
        virtual void Deactivated(RPC::IRemoteProcess* process)
        {
            _parent.Deactivated(process);
        }
        virtual void Dispatch(uint32& keyCode)
        {
            _parent.KeyEvent(keyCode);
        }
 
        BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(RPC::IRemoteProcess::INotification)
        END_INTERFACE_MAP

    private:
        Power& _parent;
    };

    class Config : public Core::JSON::Container {
    private:
        Config(const Config&);
        Config& operator=(const Config&);

    public:
        Config()
            : Core::JSON::Container()
            , OutOfProcess(true)
        {
            Add(_T("outofprocess"), &OutOfProcess);
        }
        ~Config()
        {
        }

    public:
        Core::JSON::Boolean OutOfProcess;
    };

public:
    class Data : public Core::JSON::Container {
    private:
        Data(const Data&) = delete;
        Data& operator=(const Data&) = delete;

    public:
        Data()
            : Core::JSON::Container()
            , Status()
            , PowerState()
            , Timeout()
        {
            Add(_T("Status"), &Status);
            Add(_T("PowerState"), &PowerState);
            Add(_T("Timeout"), &Timeout);
        }
        ~Data()
        {
        }

    public:
        Core::JSON::DecUInt32 Status;
        Core::JSON::DecUInt32 PowerState;
        Core::JSON::DecUInt32 Timeout;
    };

public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
    Power()
        : _skipURL(0)
        , _pid(0)
        , _power(nullptr)
        , _service(nullptr)
        , _notification(this)
    {
    }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif
    virtual ~Power()
    {
    }

public:
    BEGIN_INTERFACE_MAP(Power)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_AGGREGATE(Exchange::IPower, _power)
    END_INTERFACE_MAP

public:
    //  IPlugin methods
    // -------------------------------------------------------------------------------------------------------
    // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
    // information and services for this particular plugin. The Service object contains configuration information that
    // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
    // If there is an error, return a string describing the issue why the initialisation failed.
    // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
    // The lifetime of the Service object is guaranteed till the deinitialize method is called.
    virtual const string Initialize(PluginHost::IShell* service);

    // The plugin is unloaded from WPEFramework. This is call allows the module to notify clients
    // or to persist information if needed. After this call the plugin will unlink from the service path
    // and be deactivated. The Service object is the same as passed in during the Initialize.
    // After theis call, the lifetime of the Service object ends.
    virtual void Deinitialize(PluginHost::IShell* service);

    // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
    // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
    virtual string Information() const;

    //  IWeb methods
    // -------------------------------------------------------------------------------------------------------
    virtual void Inbound(Web::Request& request);
    virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);
    PluginHost::IShell* GetService() { return _service; }

private:
    void Deactivated(RPC::IRemoteProcess* process);
    void KeyEvent(const uint32 keyCode);

private:
    uint32 _skipURL;
    uint32 _pid;
    PluginHost::IShell* _service;
    Exchange::IPower* _power;
    Core::Sink<Notification> _notification;
};
} //namespace Plugin
} //namespace WPEFramework

#endif // __POWER_H
