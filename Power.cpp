#include "Power.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(Power, 1, 0);

static Core::ProxyPoolType<Web::JSONBodyType<Power::Data> > jsonBodyDataFactory(2);
static Core::ProxyPoolType<Web::JSONBodyType<Power::Data> > jsonResponseFactory(4);

/* virtual */ const string Power::Initialize(PluginHost::IShell* service)
{
    string message;
    Config config;

    ASSERT(_power == nullptr);
    ASSERT(_service == nullptr);

    // Setup skip URL for right offset.
    _pid = 0;
    _service = service;
    _skipURL = _service->WebPrefix().length();

    config.FromString(_service->ConfigLine());

    // Register the Process::Notification stuff. The Remote process might die before we get a change to "register" the sink for these
    // events !!!
    _service->Register (&_notification);

    if (config.OutOfProcess.Value() == true)
        _power = _service->Instantiate<Exchange::IPower>(2000, _T("PowerImplementation"), static_cast<uint32>(~0), _pid, service->Locator());
    else
        _power = Core::ServiceAdministrator::Instance().Instantiate<Exchange::IPower>(Core::Library(), _T("PowerImplementation"), static_cast<uint32>(~0));

    if (_power != nullptr) {
        TRACE(Trace::Information, (_T("Successfully instantiated Power")));
        PluginHost::VirtualInput* keyHandler (PluginHost::InputHandler::KeyHandler());

        ASSERT (keyHandler != nullptr);

        keyHandler->Register(KEY_POWER, &_notification);

    } else {
        // Seems like it is not a succes, do not forget to remove the Process:Notification registration !
        _service->Unregister(&_notification);

        _service = nullptr;

        TRACE(Trace::Error, (_T("Power could not be instantiated.")));
        message = _T("Power could not be instantiated.");
    }

    return message;
}

/* virtual */ void Power::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);
    ASSERT(_power != nullptr);

    // No need to monitor the Process::Notification anymore, we will kill it anyway.
    _service->Unregister(&_notification);

    // Also we are nolonger interested in the powerkey events, we have been requested to shut down our services!
    PluginHost::VirtualInput* keyHandler (PluginHost::InputHandler::KeyHandler());

    ASSERT (keyHandler != nullptr);

    keyHandler->Unregister(KEY_POWER, &_notification);

    // Stop processing of the browser:
    if (_power->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {

        ASSERT(_pid != 0);

        TRACE(Trace::Error, (_T("OutOfProcess Plugin is not properly destructed. PID: %d"), _pid));

        RPC::IRemoteProcess* process(_service->RemoteProcess(_pid));

        // The process can disappear in the meantime...
        if (process != nullptr) {

            // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
            process->Terminate();
            process->Release();
        }
    }

    _power = nullptr;
    _service = nullptr;
}

/* virtual */ string Power::Information() const
{
    // No additional info to report.
    return (string());
}

/* virtual */ void Power::Inbound(Web::Request& request)
{
    if (request.Verb == Web::Request::HTTP_POST)
        request.Body(jsonBodyDataFactory.Element());
}

/* virtual */ Core::ProxyType<Web::Response> Power::Process(const Web::Request& request)
{
    ASSERT(_skipURL <= request.Path.length());

    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
    Core::TextSegmentIterator index(
        Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

    result->ErrorCode = Web::STATUS_BAD_REQUEST;
    result->Message = "Unknown error";

    ASSERT (_power != nullptr);

    if ((request.Verb == Web::Request::HTTP_GET) && ((index.Next() == true) && (index.Next() == true))) {
        TRACE(Trace::Information, (string(_T("GET request"))));
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";
        if (index.Remainder() == _T("PowerState")) {
            TRACE(Trace::Information, (string(_T("PowerState"))));
            Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
            response->PowerState = _power->GetState();
            if (response->PowerState) {
                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));
            } else {
                result->Message = "Invalid State";
            }
        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = "Unknown error";
        }
    } else if ((request.Verb == Web::Request::HTTP_POST) && (index.Next() == true) && (index.Next() == true)) {
        TRACE(Trace::Information, (string(_T("POST request"))));
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";
        if (index.Remainder() == _T("PowerState")) {
            TRACE(Trace::Information, (string(_T("PowerState "))));
            uint32_t state = request.Body<const Data>()->PowerState.Value();
            uint32_t timeout = request.Body<const Data>()->Timeout.Value();
            Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
            response->Status = _power->SetState(static_cast<Exchange::IPower::PCState>(state), timeout);
            result->ContentType = Web::MIMETypes::MIME_JSON;
            result->Body(Core::proxy_cast<Web::IBody>(response));
        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = "Unknown error";
        }
    }

    return result;
}

void Power::Deactivated(RPC::IRemoteProcess* process)
{
    // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
    // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
    if (_pid == process->Id()) {
        ASSERT(_service != nullptr);
        PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
    }
}

void Power::KeyEvent(const uint32 keyCode) {

    // We only subscribed for the KEY_POWER event so do not 
    // expect anything else !!!
    ASSERT (keyCode == KEY_POWER)

    if (keyCode == KEY_POWER) {
        _power->PowerKey();
    }
}

} //namespace Plugin
} // namespace WPEFramework
