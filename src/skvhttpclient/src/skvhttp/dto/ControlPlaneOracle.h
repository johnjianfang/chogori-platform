/*
MIT License

Copyright(c) 2020 Futurewei Cloud

    Permission is hereby granted,
    free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

    The above copyright notice and this permission notice shall be included in all copies
    or
    substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS",
    WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    DAMAGES OR OTHER
    LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once

#include <skvhttp/common/Status.h>

#include <skvhttp/dto/Collection.h>
#include <skvhttp/dto/FieldTypes.h>
#include <skvhttp/dto/Timestamp.h>
#include <unordered_set>

namespace skv::http::dto {

// Request to create a collection
struct CollectionCreateRequest {
    // The metadata which describes the collection K2 should create
    CollectionMetadata metadata;
    // Only relevant for range partitioned collections. Contains the key range
    // endpoints for each partition.
    std::vector<String> rangeEnds;

    K2_SERIALIZABLE_FMT(CollectionCreateRequest, metadata, rangeEnds);
};

// Response to CollectionCreateRequest
struct CollectionCreateResponse {
  K2_SERIALIZABLE_FMT(CollectionCreateResponse);
};

// Request to get a collection
struct CollectionGetRequest {
    // The name of the collection to get
    String name;
    K2_SERIALIZABLE_FMT(CollectionGetRequest, name);
};

// Response to CollectionGetRequest
struct CollectionGetResponse {
    // The collection we found
    K2_SERIALIZABLE_FMT(CollectionGetResponse);
};

struct CollectionDropRequest {
    String name;
    K2_SERIALIZABLE_FMT(CollectionDropRequest, name);
};

struct CollectionDropResponse {
    K2_SERIALIZABLE_FMT(CollectionDropResponse);
};

struct SchemaField {
    FieldType type;
    String name;
    // Ascending or descending sort order. Currently only relevant for
    // key fields, but could be used for secondary index in the future
    bool descending = false;
    // NULL first or last in sort order. Relevant for key fields and
    // for open-ended filter predicates
    bool nullLast = false;

    K2_SERIALIZABLE_FMT(SchemaField, type, name, descending, nullLast);
};

struct Schema {
    String name;
    uint32_t version = 0;
    std::vector<SchemaField> fields;

    // All key fields must come before all value fields (by index), so that a key can be
    // constructed for a read request without knowing the schema version
    std::vector<uint32_t> partitionKeyFields;
    std::vector<uint32_t> rangeKeyFields;
    void setKeyFieldsByName(const std::vector<String>& keys, std::vector<uint32_t>& keyFields);
    void setPartitionKeyFieldsByName(const std::vector<String>& keys);
    void setRangeKeyFieldsByName(const std::vector<String>& keys);

    // Checks if the schema itself is well-formed (e.g. fields and fieldNames sizes match)
    // and returns a 400 status if not
    Status basicValidation() const;
    // Used to make sure that the partition and range key definitions do not change between versions
    Status canUpgradeTo(const dto::Schema& other) const;

    K2_SERIALIZABLE_FMT(Schema, name, version, fields, partitionKeyFields, rangeKeyFields);
};

// Request to create a schema and attach it to a collection
// If schemaName already exists, it creates a new version
struct CreateSchemaRequest {
    String collectionName;
    Schema schema;
    K2_SERIALIZABLE_FMT(CreateSchemaRequest, collectionName, schema);
};

// Response to CreateSchemaRequest
struct CreateSchemaResponse {
    K2_SERIALIZABLE_FMT(CreateSchemaResponse);
};

static inline constexpr int64_t ANY_SCHEMA_VERSION = -1;

// Request to get a schema
struct GetSchemaRequest {
    String collectionName;
    String schemaName;
    int64_t schemaVersion{ANY_SCHEMA_VERSION};

    K2_SERIALIZABLE_FMT(GetSchemaRequest, collectionName, schemaName, schemaVersion);
};

struct GetSchemaResponse {
    Schema schema;
    K2_SERIALIZABLE_FMT(GetSchemaResponse, schema);
};

// Get all versions of all schemas associated with a collection
struct GetSchemasRequest {
    String collectionName;
    K2_SERIALIZABLE_FMT(GetSchemasRequest, collectionName);
};

struct GetSchemasResponse {
    std::vector<Schema> schemas;
    K2_SERIALIZABLE_FMT(GetSchemasResponse, schemas);
};

}  // namespace skv::http::dto
