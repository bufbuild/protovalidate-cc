#include "buf/validate/internal/field_rules.h"

#include "buf/validate/internal/cel_rules.h"
#include "google/protobuf/any.pb.h"

namespace buf::validate::internal {
absl::StatusOr<std::unique_ptr<FieldConstraintRules>> BuildFieldRules(
    google::protobuf::Arena* arena,
    google::api::expr::runtime::CelExpressionBuilder& builder,
    const google::protobuf::FieldDescriptor* field,
    const FieldConstraints& fieldLvl) {
  if (fieldLvl.skipped()) {
    return nullptr;
  }
  absl::StatusOr<std::unique_ptr<FieldConstraintRules>> rules_or;
  switch (fieldLvl.type_case()) {
    case FieldConstraints::kBool:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.bool_(),
          google::protobuf::FieldDescriptor::TYPE_BOOL,
          "google.protobuf.BoolValue");
      break;
    case FieldConstraints::kFloat:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.float_(),
          google::protobuf::FieldDescriptor::TYPE_FLOAT,
          "google.protobuf.FloatValue");
      break;
    case FieldConstraints::kDouble:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.double_(),
          google::protobuf::FieldDescriptor::TYPE_DOUBLE,
          "google.protobuf.DoubleValue");
      break;
    case FieldConstraints::kInt32:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.int32(),
          google::protobuf::FieldDescriptor::TYPE_INT32,
          "google.protobuf.Int32Value");
      break;
    case FieldConstraints::kInt64:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.int64(),
          google::protobuf::FieldDescriptor::TYPE_INT64,
          "google.protobuf.Int64Value");
      break;
    case FieldConstraints::kUint32:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.uint32(),
          google::protobuf::FieldDescriptor::TYPE_UINT32,
          "google.protobuf.UInt32Value");
      break;
    case FieldConstraints::kUint64:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.uint64(),
          google::protobuf::FieldDescriptor::TYPE_UINT64,
          "google.protobuf.UInt64Value");
      break;
    case FieldConstraints::kSint32:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.sint32(),
          google::protobuf::FieldDescriptor::TYPE_SINT32);
      break;
    case FieldConstraints::kSint64:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.sint64(),
          google::protobuf::FieldDescriptor::TYPE_SINT64);
      break;
    case FieldConstraints::kFixed32:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.fixed32(),
          google::protobuf::FieldDescriptor::TYPE_FIXED32);
      break;
    case FieldConstraints::kFixed64:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.fixed64(),
          google::protobuf::FieldDescriptor::TYPE_FIXED64);
      break;
    case FieldConstraints::kSfixed32:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.sfixed32(),
          google::protobuf::FieldDescriptor::TYPE_SFIXED32);
      break;
    case FieldConstraints::kSfixed64:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.sfixed64(),
          google::protobuf::FieldDescriptor::TYPE_SFIXED64);
      break;
    case FieldConstraints::kString:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.string(),
          google::protobuf::FieldDescriptor::TYPE_STRING,
          "google.protobuf.StringValue");
      break;
    case FieldConstraints::kBytes:
      rules_or = BuildScalarFieldRules(
          arena,
          builder,
          field,
          fieldLvl,
          fieldLvl.bytes(),
          google::protobuf::FieldDescriptor::TYPE_BYTES,
          "google.protobuf.BytesValue");
      break;
    case FieldConstraints::kEnum:
      rules_or = BuildScalarFieldRules(
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
        std::unique_ptr<FieldConstraintRules> items;
        if (fieldLvl.repeated().has_items()) {
          auto items_or = BuildFieldRules(arena, builder, field, fieldLvl.repeated().items());
          if (!items_or.ok()) {
            return items_or.status();
          }
          items = std::move(items_or).value();
        }
        auto result = std::make_unique<RepeatedConstraintRules>(field, fieldLvl, std::move(items));
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
        auto keyRulesOr =
            BuildFieldRules(arena, builder, field->message_type()->field(0), fieldLvl.map().keys());
        if (!keyRulesOr.ok()) {
          return keyRulesOr.status();
        }
        auto valueRulesOr = BuildFieldRules(
            arena, builder, field->message_type()->field(1), fieldLvl.map().values());
        if (!valueRulesOr.ok()) {
          return valueRulesOr.status();
        }
        auto result = std::make_unique<MapConstraintRules>(
            field, fieldLvl, std::move(keyRulesOr).value(), std::move(valueRulesOr).value());
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
          field->message_type()->full_name() != google::protobuf::Any::descriptor()->full_name()) {
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
  if (rules_or.ok()) {
    for (const auto& constraint : fieldLvl.cel()) {
      auto status = rules_or.value()->Add(builder, constraint);
      if (!status.ok()) {
        return status;
      }
    }
  }
  return rules_or;
}

} // namespace buf::validate::internal