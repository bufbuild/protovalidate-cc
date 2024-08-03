add_custom_command(
  OUTPUT
  ${CMAKE_BINARY_DIR}/include/buf/validate/expression.pb.h
  ${CMAKE_BINARY_DIR}/include/buf/validate/expression.pb.cc
  ${CMAKE_BINARY_DIR}/include/buf/validate/priv/private.pb.h
  ${CMAKE_BINARY_DIR}/include/buf/validate/priv/private.pb.cc
  ${CMAKE_BINARY_DIR}/include/buf/validate/validate.pb.h
  ${CMAKE_BINARY_DIR}/include/buf/validate/validate.pb.cc
  COMMAND buf generate ${CMAKE_SOURCE_DIR}/third_party/protovalidate/proto/protovalidate --template ${CMAKE_SOURCE_DIR}/third_party/buf.gen.yaml --output ${CMAKE_BINARY_DIR}
  VERBATIM
)

add_custom_command(
  OUTPUT
  ${CMAKE_BINARY_DIR}/include/google/api/expr/v1alpha1/checked.pb.h
  ${CMAKE_BINARY_DIR}/include/google/api/expr/v1alpha1/checked.pb.cc
  ${CMAKE_BINARY_DIR}/include/google/api/expr/v1alpha1/syntax.pb.h
  ${CMAKE_BINARY_DIR}/include/google/api/expr/v1alpha1/syntax.pb.cc
  COMMAND buf generate ${CMAKE_SOURCE_DIR}/third_party/proto --include-imports --template ${CMAKE_SOURCE_DIR}/third_party/buf.gen.yaml --output ${CMAKE_BINARY_DIR}
  VERBATIM
)

add_library(
  validator
  validator.cc
  internal/cel_constraint_rules.cc
  internal/constraints.cc
  internal/extra_func.cc
  internal/field_rules.cc
  internal/message_rules.cc
  ${CMAKE_BINARY_DIR}/include/buf/validate/expression.pb.cc
  ${CMAKE_BINARY_DIR}/include/buf/validate/priv/private.pb.cc
  ${CMAKE_BINARY_DIR}/include/buf/validate/validate.pb.cc
  ${CMAKE_BINARY_DIR}/include/google/api/expr/v1alpha1/checked.pb.cc
  ${CMAKE_BINARY_DIR}/include/google/api/expr/v1alpha1/syntax.pb.cc
)

target_link_libraries(validator PUBLIC abseil protobuf)
target_include_directories(validator PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/protobuf/src>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/abseil-cpp>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/cel-cpp>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

install(TARGETS validator DESTINATION lib)
install(
  FILES
  validator.h
  ${CMAKE_BINARY_DIR}/include/buf/validate/expression.pb.h
  ${CMAKE_BINARY_DIR}/include/buf/validate/validate.pb.h
  DESTINATION include/buf/validate
)
install(
  FILES
  ${CMAKE_BINARY_DIR}/include/buf/validate/priv/private.pb.h
  ${CMAKE_BINARY_DIR}/include/buf/validate/priv/private.pb.cc
  DESTINATION include/buf/validate/priv
)
install(
  FILES
  internal/cel_constraint_rules.h
  internal/cel_rules.h
  internal/constraint_rules.h
  internal/constraints.h
  internal/extra_func.h
  internal/field_rules.h
  internal/message_rules.h
  internal/string_format.h
  DESTINATION include/buf/validate/internal
)
install(
  FILES
  ${CMAKE_BINARY_DIR}/include/google/api/expr/v1alpha1/checked.pb.h
  ${CMAKE_BINARY_DIR}/include/google/api/expr/v1alpha1/syntax.pb.h
  DESTINATION include/google/api/expr/v1alpha1
)
