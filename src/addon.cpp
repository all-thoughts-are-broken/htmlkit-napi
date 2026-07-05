#include <napi.h>

#include <condition_variable>
#include <exception>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "font_wrapper.h"
#include "render_engine.h"

namespace {
enum class RequestKind {
    Image,
    Css,
    UrlJoin,
};

struct BridgeRequest {
    RequestKind kind;
    std::string first;
    std::string second;

    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    bool has_value = false;
    std::vector<unsigned char> bytes;
    std::string text;
    std::string error;
};

void finish_request(BridgeRequest* request, bool has_value,
                    std::vector<unsigned char> bytes, std::string text,
                    std::string error) {
    std::unique_lock lock(request->mutex);
    if (request->done) {
        return;
    }
    request->has_value = has_value;
    request->bytes = std::move(bytes);
    request->text = std::move(text);
    request->error = std::move(error);
    request->done = true;
    lock.unlock();
    request->cv.notify_one();
}

std::string value_to_error_message(Napi::Env env, const Napi::Value& value) {
    if (value.IsObject()) {
        Napi::Object object = value.As<Napi::Object>();
        if (object.Has("message")) {
            return object.Get("message").ToString().Utf8Value();
        }
    }
    return value.ToString().Utf8Value();
}

void finish_from_value(Napi::Env env, BridgeRequest* request,
                       const Napi::Value& value) {
    if (value.IsNull() || value.IsUndefined()) {
        finish_request(request, false, {}, {}, {});
        return;
    }

    if (request->kind == RequestKind::Image) {
        if (value.IsBuffer()) {
            Napi::Buffer<unsigned char> buffer =
                value.As<Napi::Buffer<unsigned char>>();
            finish_request(
                request, true,
                std::vector<unsigned char>(buffer.Data(),
                                           buffer.Data() + buffer.Length()),
                {}, {});
            return;
        }

        if (value.IsTypedArray()) {
            Napi::TypedArray typed_array = value.As<Napi::TypedArray>();
            if (typed_array.TypedArrayType() == napi_uint8_array ||
                typed_array.TypedArrayType() == napi_uint8_clamped_array) {
                Napi::ArrayBuffer array_buffer = typed_array.ArrayBuffer();
                auto* data = static_cast<unsigned char*>(array_buffer.Data()) +
                             typed_array.ByteOffset();
                finish_request(request, true,
                               std::vector<unsigned char>(
                                   data, data + typed_array.ByteLength()),
                               {}, {});
                return;
            }
        }

        finish_request(
            request, false, {}, {},
            "imgFetch must resolve to Buffer, Uint8Array, null, or undefined");
        return;
    }

    if (!value.IsString()) {
        const char* name = request->kind == RequestKind::Css ? "cssFetch"
                                                            : "urlJoin";
        finish_request(request, false, {}, {},
                       std::string(name) +
                           " must resolve to string, null, or undefined");
        return;
    }

    finish_request(request, true, {}, value.As<Napi::String>().Utf8Value(), {});
}

void finish_from_rejection(Napi::Env env, BridgeRequest* request,
                           const Napi::Value& reason) {
    finish_request(request, false, {}, {}, value_to_error_message(env, reason));
}

Napi::Function make_resolve_handler(Napi::Env env, BridgeRequest* request) {
    return Napi::Function::New(
        env,
        [](const Napi::CallbackInfo& info) {
            auto* request = static_cast<BridgeRequest*>(info.Data());
            finish_from_value(info.Env(), request, info[0]);
            return info.Env().Undefined();
        },
        "htmlkitResolve", request);
}

Napi::Function make_reject_handler(Napi::Env env, BridgeRequest* request) {
    return Napi::Function::New(
        env,
        [](const Napi::CallbackInfo& info) {
            auto* request = static_cast<BridgeRequest*>(info.Data());
            finish_from_rejection(info.Env(), request, info[0]);
            return info.Env().Undefined();
        },
        "htmlkitReject", request);
}

void handle_callback_result(Napi::Env env, BridgeRequest* request,
                            const Napi::Value& value) {
    if (value.IsObject()) {
        Napi::Object object = value.As<Napi::Object>();
        Napi::Value then = object.Get("then");
        if (then.IsFunction()) {
            then.As<Napi::Function>().Call(
                object,
                {make_resolve_handler(env, request),
                 make_reject_handler(env, request)});
            return;
        }
    }

    finish_from_value(env, request, value);
}

void call_js_resource(Napi::Env env, Napi::Function callback,
                      BridgeRequest* request) {
    if (env == nullptr) {
        finish_request(request, false, {}, {},
                       "JavaScript environment was disposed");
        return;
    }

    try {
        Napi::Value result = request->kind == RequestKind::UrlJoin
                                 ? callback.Call(
                                       {Napi::String::New(env, request->first),
                                        Napi::String::New(env, request->second)})
                                 : callback.Call(
                                       {Napi::String::New(env, request->first)});
        handle_callback_result(env, request, result);
    } catch (const Napi::Error& error) {
        finish_request(request, false, {}, {}, error.Message());
    } catch (const std::exception& error) {
        finish_request(request, false, {}, {}, error.what());
    }
}

class NodeResourceProvider final : public ResourceProvider {
  public:
    explicit NodeResourceProvider(Napi::Env env, const Napi::Object& options) {
        maybe_create_tsfn(env, options, "imgFetch", img_fetch_, has_img_fetch_);
        maybe_create_tsfn(env, options, "cssFetch", css_fetch_, has_css_fetch_);
        maybe_create_tsfn(env, options, "urlJoin", url_join_, has_url_join_);
    }

    ~NodeResourceProvider() override {
        if (has_img_fetch_) {
            img_fetch_.Release();
        }
        if (has_css_fetch_) {
            css_fetch_.Release();
        }
        if (has_url_join_) {
            url_join_.Release();
        }
    }

    std::string join_url(const std::string& base,
                         const std::string& url) override {
        if (!has_url_join_) {
            return fallback_.join_url(base, url);
        }

        BridgeRequest request{RequestKind::UrlJoin, base, url};
        call_and_wait(url_join_, request);
        if (request.has_value) {
            return request.text;
        }
        return fallback_.join_url(base, url);
    }

    std::optional<std::vector<unsigned char>>
    fetch_image(const std::string& url) override {
        if (!has_img_fetch_) {
            return std::nullopt;
        }

        BridgeRequest request{RequestKind::Image, url, {}};
        call_and_wait(img_fetch_, request);
        if (!request.has_value) {
            return std::nullopt;
        }
        return std::move(request.bytes);
    }

    std::optional<std::string> fetch_css(const std::string& url) override {
        if (!has_css_fetch_) {
            return std::nullopt;
        }

        BridgeRequest request{RequestKind::Css, url, {}};
        call_and_wait(css_fetch_, request);
        if (!request.has_value) {
            return std::nullopt;
        }
        return std::move(request.text);
    }

  private:
    static void maybe_create_tsfn(Napi::Env env, const Napi::Object& options,
                                  const char* name,
                                  Napi::ThreadSafeFunction& target,
                                  bool& enabled) {
        enabled = false;
        if (!options.Has(name)) {
            return;
        }
        Napi::Value value = options.Get(name);
        if (value.IsNull() || value.IsUndefined()) {
            return;
        }
        if (!value.IsFunction()) {
            throw Napi::TypeError::New(env,
                                       std::string(name) + " must be a function");
        }
        target = Napi::ThreadSafeFunction::New(
            env, value.As<Napi::Function>(), name, 0, 1);
        enabled = true;
    }

    static void call_and_wait(Napi::ThreadSafeFunction& tsfn,
                              BridgeRequest& request) {
        napi_status status = tsfn.BlockingCall(&request, call_js_resource);
        if (status != napi_ok) {
            throw std::runtime_error("Failed to queue JavaScript callback");
        }

        std::unique_lock lock(request.mutex);
        request.cv.wait(lock, [&request] { return request.done; });
        if (!request.error.empty()) {
            throw std::runtime_error(request.error);
        }
    }

    NullResourceProvider fallback_;
    Napi::ThreadSafeFunction img_fetch_;
    Napi::ThreadSafeFunction css_fetch_;
    Napi::ThreadSafeFunction url_join_;
    bool has_img_fetch_ = false;
    bool has_css_fetch_ = false;
    bool has_url_join_ = false;
};

std::string get_string(const Napi::Object& object, const char* name,
                       std::string fallback) {
    if (!object.Has(name)) {
        return fallback;
    }
    Napi::Value value = object.Get(name);
    if (value.IsNull() || value.IsUndefined()) {
        return fallback;
    }
    return value.ToString().Utf8Value();
}

double get_number(const Napi::Object& object, const char* name, double fallback) {
    if (!object.Has(name)) {
        return fallback;
    }
    Napi::Value value = object.Get(name);
    if (value.IsNull() || value.IsUndefined()) {
        return fallback;
    }
    return value.ToNumber().DoubleValue();
}

bool get_bool(const Napi::Object& object, const char* name, bool fallback) {
    if (!object.Has(name)) {
        return fallback;
    }
    Napi::Value value = object.Get(name);
    if (value.IsNull() || value.IsUndefined()) {
        return fallback;
    }
    return value.ToBoolean().Value();
}

RenderOptions parse_options(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        throw Napi::TypeError::New(env, "render(html, options) expects html string");
    }

    Napi::Object options = Napi::Object::New(env);
    if (info.Length() >= 2 && !info[1].IsNull() && !info[1].IsUndefined()) {
        if (!info[1].IsObject()) {
            throw Napi::TypeError::New(env, "options must be an object");
        }
        options = info[1].As<Napi::Object>();
    }

    RenderOptions render_options;
    render_options.html = info[0].As<Napi::String>().Utf8Value();
    render_options.base_url = get_string(options, "baseUrl", "");
    render_options.dpi = static_cast<float>(get_number(options, "dpi", 96.0));
    render_options.width =
        static_cast<float>(get_number(options, "width",
                                      get_number(options, "maxWidth", 800.0)));
    render_options.height =
        static_cast<float>(get_number(options, "height",
                                      get_number(options, "deviceHeight", 600.0)));
    render_options.default_font_size =
        static_cast<float>(get_number(options, "defaultFontSize", 12.0));
    render_options.font_name = get_string(options, "fontName", "sans-serif");
    render_options.allow_refit = get_bool(options, "allowRefit", true);
    render_options.language = get_string(options, "lang", "zh");
    render_options.culture = get_string(options, "culture", "CN");
    render_options.native_data_scheme =
        get_bool(options, "nativeDataScheme", true);
    render_options.debug = get_bool(options, "debug", false);

    std::string image_format = get_string(options, "imageFormat", "png");
    if (image_format == "jpeg" || image_format == "jpg") {
        render_options.image_flag =
            static_cast<int>(get_number(options, "jpegQuality", 100));
    } else {
        render_options.image_flag = -1;
    }
    if (options.Has("imageFlag")) {
        render_options.image_flag =
            options.Get("imageFlag").ToNumber().Int32Value();
    }

    return render_options;
}

class RenderWorker final : public Napi::AsyncWorker {
  public:
    RenderWorker(Napi::Env env, RenderOptions options, Napi::Object js_options,
                 Napi::Promise::Deferred deferred)
        : Napi::AsyncWorker(env), options_(std::move(options)),
          provider_(env, js_options), deferred_(deferred) {}

    void Execute() override {
        try {
            result_ = render_html(options_, &provider_);
        } catch (const std::exception& error) {
            SetError(error.what());
        } catch (...) {
            SetError("Unknown native render error");
        }
    }

    void OnOK() override {
        Napi::Env env = Env();
        Napi::Buffer<unsigned char> image =
            Napi::Buffer<unsigned char>::Copy(env, result_.image.data(),
                                              result_.image.size());
        if (result_.debug_html) {
            Napi::Object object = Napi::Object::New(env);
            object.Set("image", image);
            object.Set("debugHtml", *result_.debug_html);
            deferred_.Resolve(object);
        } else {
            deferred_.Resolve(image);
        }
    }

    void OnError(const Napi::Error& error) override {
        deferred_.Reject(error.Value());
    }

  private:
    RenderOptions options_;
    NodeResourceProvider provider_;
    Napi::Promise::Deferred deferred_;
    RenderResult result_;
};

Napi::Value render(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    try {
        RenderOptions options = parse_options(info);
        Napi::Object js_options = Napi::Object::New(env);
        if (info.Length() >= 2 && info[1].IsObject()) {
            js_options = info[1].As<Napi::Object>();
        }

        auto deferred = Napi::Promise::Deferred::New(env);
        auto* worker =
            new RenderWorker(env, std::move(options), js_options, deferred);
        worker->Queue();
        return deferred.Promise();
    } catch (const Napi::Error& error) {
        error.ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value init_fontconfig_binding(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (init_fontconfig() != 0) {
        throw Napi::Error::New(env, "Failed to initialize fontconfig");
    }
    return env.Undefined();
}

Napi::Object init(Napi::Env env, Napi::Object exports) {
    exports.Set("render", Napi::Function::New(env, render));
    exports.Set("initFontconfig",
                Napi::Function::New(env, init_fontconfig_binding));
    return exports;
}
} // namespace

NODE_API_MODULE(htmlkit, init)
