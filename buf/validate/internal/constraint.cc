#include "buf/validate/internal/constraint.h"

#include "absl/status/statusor.h"
#include "buf/validate/internal/extra_func.h"
#include "buf/validate/priv/private.pb.h"
#include "buf/validate/validate.pb.h"
#include "eval/public/builtin_func_registrar.h"
#include "eval/public/cel_expr_builder_factory.h"
#include "eval/public/cel_value.h"
#include "eval/public/containers/field_access.h"
#include "eval/public/containers/field_backed_list_impl.h"
#include "eval/public/containers/field_backed_map_impl.h"
#include "eval/public/structs/cel_proto_wrapper.h"
#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.pb.h"

namespace buf::validate::internal {
namespace cel = google::api::expr;

namespace {

absl::StatusOr<std::unique_ptr<MessageConstraintRules>> BuildMessageRules(
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const MessageConstraints& constraints) {
  auto result = std::make_unique<MessageConstraintRules>();
  for (const auto& constraint : constraints.cel()) {
    if (auto status = result->Add(builder, constraint); !status.ok()) {
      return status;
    }
  }
  return result;
}

} // namespace

absl::StatusOr<std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder>>
NewConstraintBuilder(google::protobuf::Arena* arena) {
  cel::runtime::InterpreterOptions options;
  options.enable_qualified_type_identifiers = true;
  options.enable_timestamp_duration_overflow_errors = true;
  options.enable_heterogeneous_equality = true;
  options.enable_empty_wrapper_null_unboxing = true;
  options.enable_regex_precompilation = true;
  options.constant_folding = true;
  options.constant_arena = arena;

  std::unique_ptr<cel::runtime::CelExpressionBuilder> builder =
      cel::runtime::CreateCelExpressionBuilder(options);
  auto register_status = cel::runtime::RegisterBuiltinFunctions(builder->GetRegistry(), options);
  if (!register_status.ok()) {
    return register_status;
  }
  register_status = RegisterExtraFuncs(*builder->GetRegistry(), arena);
  if (!register_status.ok()) {
    return register_status;
  }
  return builder;
}

template <typename R>
absl::Status BuildCelRules(
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const R& rules,
    CelConstraintRules& result) {
  result.setRules(&rules, arena);
  // Look for constraints on the set fields.
  std::vector<const google::protobuf::FieldDescriptor*> fields;
  R::GetReflection()->ListFields(rules, &fields);
  for (const auto* field : fields) {
    if (!field->options().HasExtension(buf::validate::priv::field)) {
      continue;
    }
    const auto& fieldLvl = field->options().GetExtension(buf::validate::priv::field);
    for (const auto& constraint : fieldLvl.cel()) {
      auto status =
          result.Add(builder, constraint.id(), constraint.message(), constraint.expression());
      if (!status.ok()) {
        return status;
      }
    }
  }
  return absl::OkStatus();
}

template <typename R>
absl::StatusOr<std::unique_ptr<FieldConstraintRules>> BuildFieldRules(
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::FieldDescriptor* field,
    const FieldConstraints& fieldLvl,
    const R& rules,
    google::protobuf::FieldDescriptor::Type expectedType,
    std::string_view wrapperName = "") {
  if (field->type() != expectedType) {
    if (field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
      if (field->message_type()->full_name() != wrapperName) {
        return absl::FailedPreconditionError(absl::StrFormat(
            "field type does not match constraint type: %s != %s",
            field->type_name(),
            google::protobuf::FieldDescriptor::TypeName(expectedType)));
      }
    } else {
      return absl::FailedPreconditionError(absl::StrFormat(
          "field type does not match constraint type: %s != %s",
          google::protobuf::FieldDescriptor::TypeName(field->type()),
          google::protobuf::FieldDescriptor::TypeName(expectedType)));
    }
  }
  auto result = std::make_unique<FieldConstraintRules>(field, fieldLvl);
  auto status = BuildCelRules(arena, builder, rules, *result);
  if (!status.ok()) {
    return status;
  }
  return result;
}

Constraints NewMessageConstraints(
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::Descriptor* descriptor) {
  std::vector<std::unique_ptr<ConstraintRules>> result;
  if (descriptor->options().HasExtension(buf::validate::message)) {
    const auto& msgLvl = descriptor->options().GetExtension(buf::validate::message);
    if (msgLvl.disabled()) {
      return result;
    }
    auto rules_or = BuildMessageRules(builder, msgLvl);
    if (!rules_or.ok()) {
      return rules_or.status();
    }
    result.emplace_back(std::move(rules_or).value());
  }

  for (int i = 0; i < descriptor->field_count(); i++) {
    const google::protobuf::FieldDescriptor* field = descriptor->field(i);
    if (!field->options().HasExtension(buf::validate::field)) {
      continue;
    }
    const auto& fieldLvl = field->options().GetExtension(buf::validate::field);
    if (fieldLvl.skipped()) {
      continue;
    }
    absl::StatusOr<std::unique_ptr<CelConstraintRules>> rules_or;
    switch (fieldLvl.type_case()) {
      case FieldConstraints::kBool:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.bool_(),
            google::protobuf::FieldDescriptor::TYPE_BOOL,
            "google.protobuf.BoolValue");
        break;
      case FieldConstraints::kFloat:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.float_(),
            google::protobuf::FieldDescriptor::TYPE_FLOAT,
            "google.protobuf.FloatValue");
        break;
      case FieldConstraints::kDouble:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.double_(),
            google::protobuf::FieldDescriptor::TYPE_DOUBLE,
            "google.protobuf.DoubleValue");
        break;
      case FieldConstraints::kInt32:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.int32(),
            google::protobuf::FieldDescriptor::TYPE_INT32,
            "google.protobuf.Int32Value");
        break;
      case FieldConstraints::kInt64:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.int64(),
            google::protobuf::FieldDescriptor::TYPE_INT64,
            "google.protobuf.Int64Value");
        break;
      case FieldConstraints::kUint32:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.uint32(),
            google::protobuf::FieldDescriptor::TYPE_UINT32,
            "google.protobuf.UInt32Value");
        break;
      case FieldConstraints::kUint64:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.uint64(),
            google::protobuf::FieldDescriptor::TYPE_UINT64,
            "google.protobuf.UInt64Value");
        break;
      case FieldConstraints::kSint32:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.sint32(),
            google::protobuf::FieldDescriptor::TYPE_SINT32);
        break;
      case FieldConstraints::kSint64:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.sint64(),
            google::protobuf::FieldDescriptor::TYPE_SINT64);
        break;
      case FieldConstraints::kFixed32:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.fixed32(),
            google::protobuf::FieldDescriptor::TYPE_FIXED32);
        break;
      case FieldConstraints::kFixed64:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.fixed64(),
            google::protobuf::FieldDescriptor::TYPE_FIXED64);
        break;
      case FieldConstraints::kSfixed32:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.sfixed32(),
            google::protobuf::FieldDescriptor::TYPE_SFIXED32);
        break;
      case FieldConstraints::kSfixed64:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.sfixed64(),
            google::protobuf::FieldDescriptor::TYPE_SFIXED64);
        break;
      case FieldConstraints::kString:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.string(),
            google::protobuf::FieldDescriptor::TYPE_STRING,
            "google.protobuf.StringValue");
        break;
      case FieldConstraints::kBytes:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.bytes(),
            google::protobuf::FieldDescriptor::TYPE_BYTES,
            "google.protobuf.BytesValue");
        break;
      case FieldConstraints::kEnum:
        rules_or = BuildFieldRules(
            arena,
            builder,
            field,
            fieldLvl,
            fieldLvl.enum_(),
            google::protobuf::FieldDescriptor::TYPE_ENUM);
        break;
      case FieldConstraints::kDuration:
        if (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE ||
            field->message_type()->full_name() !=
                google::protobuf::Duration::descriptor()->full_name()) {
          return absl::InvalidArgumentError("duration field validator on non-duration field");
        } else {
          auto result = std::make_unique<FieldConstraintRules>(field, fieldLvl);
          auto status = BuildCelRules(arena, builder, fieldLvl.duration(), *result);
          if (!status.ok()) {
            rules_or = status;
          } else {
            rules_or = std::move(result);
          }
        }
        break;
      case FieldConstraints::kTimestamp:
        if (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE ||
            field->message_type()->full_name() !=
                google::protobuf::Timestamp::descriptor()->full_name()) {
          return absl::InvalidArgumentError("timestamp field validator on non-timestamp field");
        } else {
          auto result = std::make_unique<FieldConstraintRules>(field, fieldLvl);
          auto status = BuildCelRules(arena, builder, fieldLvl.timestamp(), *result);
          if (!status.ok()) {
            rules_or = status;
          } else {
            rules_or = std::move(result);
          }
        }
        break;
      case FieldConstraints::kRepeated:
        if (!field->is_repeated()) {
          return absl::InvalidArgumentError("repeated field validator on non-repeated field");
        } else if (field->is_map()) {
          return absl::InvalidArgumentError("repeated field validator on map field");
        } else {
          auto result = std::make_unique<FieldConstraintRules>(field, fieldLvl);
          auto status = BuildCelRules(arena, builder, fieldLvl.repeated(), *result);
          if (!status.ok()) {
            rules_or = status;
          } else {
            rules_or = std::move(result);
          }
        }
        break;
      case FieldConstraints::kMap:
        if (!field->is_map()) {
          return absl::InvalidArgumentError("map field validator on non-map field");
        } else {
          auto result = std::make_unique<FieldConstraintRules>(field, fieldLvl);
          auto status = BuildCelRules(arena, builder, fieldLvl.map(), *result);
          if (!status.ok()) {
            rules_or = status;
          } else {
            rules_or = std::move(result);
          }
        }
        break;
      case FieldConstraints::kAny:
        if (field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE ||
            field->message_type()->full_name() !=
                google::protobuf::Any::descriptor()->full_name()) {
          return absl::InvalidArgumentError("any field validator on non-any field");
        } else {
          auto result = std::make_unique<FieldConstraintRules>(field, fieldLvl, &fieldLvl.any());
          auto status = BuildCelRules(arena, builder, fieldLvl.any(), *result);
          if (!status.ok()) {
            rules_or = status;
          } else {
            rules_or = std::move(result);
          }
        }
        break;
      case FieldConstraints::TYPE_NOT_SET:
        rules_or = std::make_unique<FieldConstraintRules>(field, fieldLvl);
        break;
      default:
        return absl::InvalidArgumentError(
            absl::StrFormat("unknown field validator type %d", fieldLvl.type_case()));
    }
    if (!rules_or.ok()) {
      return rules_or.status();
    }
    for (const auto& constraint : fieldLvl.cel()) {
      auto status = rules_or.value()->Add(builder, constraint);
      if (!status.ok()) {
        return status;
      }
    }
    result.emplace_back(std::move(rules_or).value());
  }

  for (int i = 0; i < descriptor->oneof_decl_count(); i++) {
    const google::protobuf::OneofDescriptor* oneof = descriptor->oneof_decl(i);
    if (!oneof->options().HasExtension(buf::validate::oneof)) {
      continue;
    }
    const auto& oneofLvl = oneof->options().GetExtension(buf::validate::oneof);
    result.emplace_back(std::make_unique<OneofConstraintRules>(oneof, oneofLvl));
  }

  return result;
}

absl::Status MessageConstraintRules::Validate(
    ConstraintContext& ctx,
    std::string_view fieldPath,
    const google::protobuf::Message& message) const {
  google::api::expr::runtime::Activation activation;
  activation.InsertValue("this", cel::runtime::CelProtoWrapper::CreateMessage(&message, ctx.arena));
  return ValidateCel(ctx, fieldPath, activation);
}

absl::Status FieldConstraintRules::Validate(
    ConstraintContext& ctx,
    std::string_view fieldPath,
    const google::protobuf::Message& message) const {
  google::api::expr::runtime::Activation activation;
  cel::runtime::CelValue result;
  std::string subPath;
  if (fieldPath.empty()) {
    fieldPath = field_->name();
  } else {
    subPath = absl::StrCat(fieldPath, ".", field_->name());
    fieldPath = subPath;
  }
  if (field_->is_map()) {
    result = cel::runtime::CelValue::CreateMap(
        google::protobuf::Arena::Create<cel::runtime::FieldBackedMapImpl>(
            ctx.arena, &message, field_, ctx.arena));
  } else if (field_->is_repeated()) {
    result = cel::runtime::CelValue::CreateList(
        google::protobuf::Arena::Create<cel::runtime::FieldBackedListImpl>(
            ctx.arena, &message, field_, ctx.arena));
  } else {
    if (!message.GetReflection()->HasField(message, field_)) {
      if (required_) {
        auto& violation = *ctx.violations.add_violations();
        *violation.mutable_constraint_id() = "required";
        *violation.mutable_message() = "value is required";
        *violation.mutable_field_path() = fieldPath;
        return absl::OkStatus();
      } else if (
          ignoreEmpty_ || field_->containing_oneof() != nullptr ||
          field_->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
        return absl::OkStatus();
      }
    } else if (
        anyRules_ != nullptr &&
        field_->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      const auto& anyMsg = message.GetReflection()->GetMessage(message, field_);
      auto status = ValidateAny(ctx, fieldPath, anyMsg);
      if (!status.ok()) {
        return status;
      }
    }

    auto status = cel::runtime::CreateValueFromSingleField(&message, field_, ctx.arena, &result);
    if (!status.ok()) {
      return status;
    }
  }
  activation.InsertValue("this", result);
  return ValidateCel(ctx, fieldPath, activation);
}

absl::Status FieldConstraintRules::ValidateAny(
    ConstraintContext& ctx,
    std::string_view fieldPath,
    const google::protobuf::Message& anyMsg) const {
  const auto* typeUriField = anyMsg.GetDescriptor()->FindFieldByName("type_url");
  if (typeUriField == nullptr ||
      typeUriField->type() != google::protobuf::FieldDescriptor::TYPE_STRING) {
    return absl::InvalidArgumentError("expected Any");
  }
  const auto& typeUri = anyMsg.GetReflection()->GetString(anyMsg, typeUriField);
  if (anyRules_->in_size() > 0) {
    // Must be in the list of allowed types.
    bool found = false;
    for (const auto& allowed : anyRules_->in()) {
      if (allowed == typeUri) {
        found = true;
        break;
      }
    }
    if (!found) {
      auto& violation = *ctx.violations.add_violations();
      *violation.mutable_constraint_id() = "any.in";
      *violation.mutable_message() = "type URL must be in the allow list";
      *violation.mutable_field_path() = fieldPath;
    }
  }
  for (const auto& block : anyRules_->not_in()) {
    if (block == typeUri) {
      auto& violation = *ctx.violations.add_violations();
      *violation.mutable_constraint_id() = "any.not_in";
      *violation.mutable_message() = "type URL must not be in the block list";
      *violation.mutable_field_path() = fieldPath;
      break;
    }
  }
  return absl::OkStatus();
}

absl::Status OneofConstraintRules::Validate(
    buf::validate::internal::ConstraintContext& ctx,
    std::string_view fieldPath,
    const google::protobuf::Message& message) const {
  std::string subPath;
  if (fieldPath.empty()) {
    fieldPath = oneof_->name();
  } else {
    subPath = absl::StrCat(fieldPath, ".", oneof_->name());
    fieldPath = subPath;
  }
  if (required_) {
    if (!message.GetReflection()->HasOneof(message, oneof_)) {
      auto& violation = *ctx.violations.add_violations();
      *violation.mutable_constraint_id() = "required";
      *violation.mutable_message() = "exactly one of oneof fields is required";
      *violation.mutable_field_path() = fieldPath;
    }
  }
  return absl::OkStatus();
}

} // namespace buf::validate::internal
