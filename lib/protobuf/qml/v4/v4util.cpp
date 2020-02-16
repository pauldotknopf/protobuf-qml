#include "protobuf/qml/v4/v4util.h"

#ifndef PROTOBUF_QML_DISABLE_SERIALIZE
#include <private/qqmlcontextwrapper_p.h>
#endif

using namespace QV4;

namespace protobuf {
namespace qml {

QV4::Heap::ArrayBuffer* allocate_array_buffer(QV4::ExecutionEngine* v4,
                                              size_t size) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
  return v4->newArrayBuffer(QByteArray(size, 0));
#else
  return v4->memoryManager->alloc<ArrayBuffer>(v4, size);
#endif
}

QQmlContextData* callingQmlContext(ExecutionEngine* v4) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
  return v4->callingQmlContext();
#else
  return v4->v8Engine->callingContext();
#endif
}

#ifndef PROTOBUF_QML_DISABLE_SERIALIZE

ReturnedValue packCallbackObject(ExecutionEngine* v4, const Value& callback) {
  Scope scope(v4);
  ScopedObject o(scope, v4->newObject());
  ScopedValue ctx(scope, QmlContextWrapper::qmlScope(scope.engine,
                                                     callingQmlContext(v4), 0));
  o->put(ScopedString(scope, v4->newString(QStringLiteral("CallingContext"))),
         ctx);
  ScopedFunctionObject cb(scope, callback);
  o->put(ScopedString(scope, v4->newString(QStringLiteral("CallbackObject"))),
         cb);
  return o->asReturnedValue();
}

std::pair<ReturnedValue, ReturnedValue> unpackCallbackObject(
    ExecutionEngine* v4, ReturnedValue packed) {
#define UNPACK_EMPTY_RESULT std::make_pair(Encode::null(), Encode::null())
  Scope scope(v4);
  ScopedObject o(scope, packed);
  if (!o) {
    // v4->throwError(QStringLiteral("No callback"));
    return UNPACK_EMPTY_RESULT;
  }
  ScopedString s(scope, v4->newString(QStringLiteral("CallingContext")));
  ScopedObject ctx(scope, o->get(s));
  if (!ctx) {
    qDebug() << "Calling context is no longer available";
    return UNPACK_EMPTY_RESULT;
  }
  s = v4->newString(QStringLiteral("CallbackObject"));
  ScopedObject cb(scope, o->get(s));
  if (!cb) {
    qDebug() << "No callback";
    return UNPACK_EMPTY_RESULT;
  }
  Scoped<QmlContextWrapper> c(scope, *ctx);
  auto contextData = c->getContext();
  if (!contextData) {
    qDebug() << "Calling context is no longer available";
    return UNPACK_EMPTY_RESULT;
  }
  return std::make_pair(ctx.asReturnedValue(), cb.asReturnedValue());
#undef UNPACK_EMPTY_RESULT
}

#endif

}
}
