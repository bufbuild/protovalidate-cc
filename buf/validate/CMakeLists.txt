add_custom_command(
  OUTPUT
  ${CMAKE_BINARY_DIR}/include/buf/validate/expression.pb.h
  ${CMAKE_BINARY_DIR}/include/buf/validate/expression.pb.cc
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
  ${CMAKE_BINARY_DIR}/include/buf/validate/expression.pb.cc
  ${CMAKE_BINARY_DIR}/include/buf/validate/validate.pb.cc
  ${CMAKE_BINARY_DIR}/include/google/api/expr/v1alpha1/checked.pb.cc
  ${CMAKE_BINARY_DIR}/include/google/api/expr/v1alpha1/syntax.pb.cc
)

target_link_libraries(validator PUBLIC protobuf)
target_include_directories(validator PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/protobuf/src>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/abseil-cpp>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/cel-cpp>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

install(TARGETS validator DESTINATION lib)
install(FILES validator.h DESTINATION include/buf/validate)
install(FILES ${CMAKE_BINARY_DIR}/include/buf/validate/expression.pb.h DESTINATION include/buf/validate)
