/* ***** BEGIN LICENSE BLOCK *****
 * Distributed under the BSD license:
 *
 * Copyright (c) 2015, xuewen.chu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of xuewen.chu nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL xuewen.chu BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ***** END LICENSE BLOCK ***** */

#include "ngui/base/string.h"
#include "ngui/base/fs.h"
#include "ngui/base/sys.h"
#include "ngui/base/loop.h"
#include "ngui/base/string-builder.h"
#include "ngui/js/ngui.h"
#include "ngui/base/jsx.h"
#include "ngui/js/str.h"
#include "native-core-js.h"
#include "json-1.h"

/**
 * @ns ngui::js
 */

JS_BEGIN

typedef Object NativeObject;
static cString Space(' ');

class WrapNativeObject: public WrapObject {
  static void constructor(FunctionCall args) {
    JS_ATTACH(args);
    New<WrapNativeObject>(args, new NativeObject());
  }
 public:
  static void binding(Local<JSObject> exports, Worker* worker) {
    JS_DEFINE_CLASS_NO_EXPORTS(NativeObject, constructor, {
      // none
    }, nullptr);
  }
};

/**
 * @class WrapSimpleHash
 */
class WrapSimpleHash: public WrapObject {
 public:
  
  static void constructor(FunctionCall args) {
    New<WrapSimpleHash>(args, new SimpleHash());
  }
  
  static void hashCode(FunctionCall args) {
    JS_WORKER(args);
    JS_SELF(SimpleHash);
    JS_RETURN( self->hash_code() );
  }
  
  static void update(FunctionCall args) {
    JS_WORKER(args);
    if (  args.Length() < 1 ||
        !(args[0]->IsString(worker) || worker->has_buffer(args[0]))
    ) {
      JS_THROW_ERR("Bad argument");
    }
    JS_SELF(SimpleHash);
    
    if ( args[0]->IsString(worker) ) { // 字符串
      Ucs2String str = args[0]->ToUcs2StringValue(worker);
      self->update(*str, str.length());
    }
    else { // Buffer
      WeakBuffer buff = worker->as_buffer(args[0]);
      self->update(*buff, buff.length());
    }
  }
  
  static void digest(FunctionCall args) {
    JS_WORKER(args);
    JS_SELF(SimpleHash);
    JS_RETURN( self->digest() );
  }
  
  static void clear(FunctionCall args) {
    JS_SELF(SimpleHash);
    self->clear();
  }

  /**
   * @func binding
   */
  static void binding(Local<JSObject> exports, Worker* worker) {
    JS_DEFINE_CLASS(SimpleHash, constructor, {
      JS_SET_CLASS_METHOD(hashCode, hashCode);
      JS_SET_CLASS_METHOD(update, update);
      JS_SET_CLASS_METHOD(digest, digest);
      JS_SET_CLASS_METHOD(clear, clear);
    }, nullptr);
  }
};

/**
 * @class NativeUtil
 */
class NativeUtil {
 public:

  static SimpleHash get_hash_code(FunctionCall args) {
    JS_WORKER(args);
    SimpleHash hash;
    Ucs2String str = args[0]->ToUcs2StringValue(worker);
    hash.update(*str, str.length());
    return hash;
  }
  
  // api
  static void fatal(FunctionCall args) {
    JS_WORKER(args);
    if ( args.Length() ) {
      worker->fatal(args[0]);
    } else {
      worker->fatal(worker->New("Fatal"));
    }
  }
  
  static void hashCode(FunctionCall args) {
    JS_WORKER(args);
    if (args.Length() < 1 || ! args[0]->IsString(worker)) {
      JS_THROW_ERR("Bad argument");
    }
    JS_RETURN( get_hash_code(args).hash_code() );
  }
  
  static void hash(FunctionCall args) {
    JS_WORKER(args);
    if (args.Length() < 1 || ! args[0]->IsString(worker)) {
      JS_THROW_ERR("Bad argument");
    }
    JS_RETURN( get_hash_code(args).digest() );
  }
  
  static void version(FunctionCall args) {
    JS_WORKER(args);
    JS_RETURN( ngui::version() );
  }
  
  static void addNativeEventListener(FunctionCall args) {
    JS_WORKER(args);
    if ( args.Length() < 3 || !args[0]->IsObject(worker) ||
        !args[1]->IsString(worker) || !args[2]->IsFunction(worker)) {
      JS_THROW_ERR("Bad argument");
    }
    if ( ! WrapObject::is_pack(args[0].To<JSObject>()) ) {
      JS_THROW_ERR("Bad argument");
    }
    int id = 0;
    if ( args.Length() > 3 && args[3]->IsNumber(worker) ) {
      id = args[3]->ToNumberValue(worker);
    }
    { HandleScope scope(worker);
      WrapObject* wrap = WrapObject::unpack(args[0].To<JSObject>());
      String name = args[1]->ToStringValue(worker,1);
      String func = String("__on").push(name).push("_native");
      wrap->set(worker->New(func,1), args[2]);
      bool ok = wrap->add_event_listener(name, func, id);
      JS_RETURN(ok);
    }
  }
  
  static void removeNativeEventListener(FunctionCall args) {
    JS_WORKER(args);
    if ( args.Length() < 2 || !args[0]->IsObject(worker) || !args[1]->IsString(worker)) {
      JS_THROW_ERR("Bad argument");
    }
    if ( ! WrapObject::is_pack(args[0].To<JSObject>()) ) {
      JS_THROW_ERR("Bad argument");
    }
    int id = 0;
    if ( args.Length() > 2 && args[2]->IsNumber(worker) ) {
      id = args[2]->ToNumberValue(worker);
    }
    { HandleScope scope(worker);
      String name = args[1]->ToStringValue(worker,1);
      WrapObject* wrap = WrapObject::unpack(args[0].To<JSObject>());
      bool ok = wrap->remove_event_listener(name, id);
      if ( ok ) {
        String func = String("__on").push(name).push("_native");
        wrap->del( worker->New(func) );
      }
      JS_RETURN(ok);
    }
  }

  static void garbageCollection(FunctionCall args) {
    JS_WORKER(args); GUILock lock;
    worker->garbage_collection();
#if XX_MEMORY_TRACE_MARK
    std::vector<Object*> objs = Object::mark_objects();
    Object** objs2 = &objs[0];
    LOG("All unrelease heap objects count: %d", objs.size());
#endif
  }
  
  static void run_script(FunctionCall args) {
    JS_WORKER(args);
    if (args.Length() < 1 || ! args[0]->IsString(worker)) {
      JS_THROW_ERR("Bad argument");
    }
    JS_HANDLE_SCOPE();
    Local<JSString> name;
    Local<JSObject> sandbox;
    if (args.Length() > 1) {
      name = args[1]->ToString(worker);
    } else {
      name = worker->New("[eval]",1);
    }
    if (args.Length() > 2 && args[2]->IsObject(worker)) {
      sandbox = args[2].To<JSObject>();
    }
    Local<JSValue> rv = worker->run_script(args[0].To<JSString>(), name, sandbox);
    if ( !rv.IsEmpty() ) { // 没有值可能有异常
      JS_RETURN( rv );
    }
  }
  
  static void transform_js(FunctionCall args, bool jsx) {
    JS_WORKER(args);
    if (args.Length() < 2 || !args[0]->IsString(worker) || !args[1]->IsString(worker)) {
      JS_THROW_ERR("Bad argument");
    }
    Ucs2String rv;
    Ucs2String in = args[0]->ToUcs2StringValue(worker);
    String path = args[1]->ToStringValue(worker);
    JS_TRY_CATCH({
      if (jsx) {
        rv = Jsx::transform_jsx(in, path);
      } else {
        rv = Jsx::transform_js(in, path);
      }
    }, Error);
    
    JS_RETURN( rv );
  }
  
  static void transformJsx(FunctionCall args) {
    transform_js(args, true);
  }
  
  static void transformJs(FunctionCall args) {
    transform_js(args, false);
  }
  
  /**
   * @func executable()
   * @ret {String}
   */
  static void executable(FunctionCall args) {
    JS_WORKER(args);
    JS_RETURN( Path::executable() );
  }
  
  /**
   * @func documents([path])
   * @arg path {String}
   * @ret {String}
   */
  static void documents(FunctionCall args) {
    JS_WORKER(args);
    if (args.Length() == 0 || !args[0]->IsString(worker)) {
      JS_RETURN( Path::documents() );
    }
    JS_RETURN( Path::documents( args[0]->ToStringValue(worker)) );
  }
  
  /**
   * @func temp([path])
   * @arg path {String}
   * @ret {String}
   */
  static void temp(FunctionCall args) {
    JS_WORKER(args);
    if (args.Length() == 0 || !args[0]->IsString(worker)) {
      JS_RETURN( Path::temp() );
    }
    JS_RETURN( Path::temp( args[0]->ToStringValue(worker)) );
  }
  
  /**
   * @func resources([path])
   * @arg path {String}
   * @ret {String}
   */
  static void resources(FunctionCall args) {
    JS_WORKER(args);
    if (args.Length() == 0 || !args[0]->IsString(worker)) {
      JS_RETURN( Path::resources() );
    }
    JS_RETURN( Path::resources( args[0]->ToStringValue(worker)) );
  }
  
  /**
   * @func fallbackPath(path)
   * @arg path {String}
   * @ret {String}
   */
  static void fallbackPath(FunctionCall args) {
    JS_WORKER(args);
    if (args.Length() == 0 || !args[0]->IsString(worker)) {
      JS_THROW_ERR(
                   "* @func fallbackPath(path)\n"
                   "* @arg path {String}\n"
                   "* @ret {String}\n"
                   );
    }
    JS_RETURN( Path::fallback(args[0]->ToStringValue(worker)) );
  }
  
  /**
   * @func cwd()
   * @ret {String}
   */
  static void cwd(FunctionCall args) {
    JS_WORKER(args);
    JS_RETURN( Path::cwd() );
  }
  
  /**
   * @func chdir(path)
   * @arg path {String}
   * @ret {bool}
   */
  static void chdir(FunctionCall args) {
    JS_WORKER(args);
    if (args.Length() == 0 || !args[0]->IsString(worker)) {
      JS_RETURN( false );
    }
    JS_RETURN( Path::chdir(args[0]->ToStringValue(worker)) );
  }
  
  static void log(FunctionCall args) {
    JS_WORKER(args);
    StringBuilder rv;
    bool is_space = false;
    
    for (int i = 0; i < args.Length(); i++) {
      if (is_space) {
        rv.push(Space);
      }
      if (args[i]->IsObject(worker)) {
        if (!JSON::stringify_console_styled(worker, args[i], &rv)) {
          return;
        }
      } else {
        rv.push( args[i]->ToStringValue(worker) );
      }
      is_space = true;
    }
    console::log(rv.to_string());
  }
  
  /**
   * @func binding
   */
  static void binding(Local<JSObject> exports, Worker* worker) {
    JS_SET_METHOD(fatal, fatal);
    JS_SET_METHOD(hashCode, hashCode);
    JS_SET_METHOD(hash, hash);
    JS_SET_METHOD(version, version);
    JS_SET_METHOD(addNativeEventListener, addNativeEventListener);
    JS_SET_METHOD(removeNativeEventListener, removeNativeEventListener);
    JS_SET_METHOD(runScript, run_script);
    JS_SET_METHOD(garbageCollection, garbageCollection);
    JS_SET_METHOD(transformJsx, transformJsx);
    JS_SET_METHOD(transformJs, transformJs);
    JS_SET_METHOD(executable, executable);
    JS_SET_METHOD(documents, documents);
    JS_SET_METHOD(temp, temp);
    JS_SET_METHOD(resources, resources);
    JS_SET_METHOD(fallbackPath, fallbackPath);
    JS_SET_METHOD(cwd, cwd);
    JS_SET_METHOD(chdir, chdir);
    JS_SET_METHOD(log, log);
    WrapNativeObject::binding(exports, worker);
    WrapSimpleHash::binding(exports, worker);
  }
};

JS_REG_MODULE(ngui_util, NativeUtil)
JS_END
