# VectorSearcher Integration Guide

This document describes how to integrate the VectorSearcher class with SemanticDatabase for vector similarity search.

## Overview

The VectorSearcher class wraps the sqlite-vec extension to provide vector similarity search on 384-dimensional embeddings. It must be integrated with the SemanticDatabase to enable semantic search functionality.

## Integration Steps

### 1. Initialize Extension in SemanticDatabase::Open()

Add extension initialization when opening the database:

```cpp
// In SemanticDatabase.cpp - Open() method
bool CSemanticDatabase::Open()
{
  // ... existing database open code ...

  // Initialize sqlite-vec extension
  CVectorSearcher vectorSearcher;
  if (!vectorSearcher.InitializeExtension(m_pDB->getHandle()))
  {
    CLog::Log(LOGWARNING, "CSemanticDatabase::{}: Failed to initialize sqlite-vec extension", __func__);
    // Vector search will not be available, but database can still function
  }

  // Create vector table
  if (!vectorSearcher.CreateVectorTable())
  {
    CLog::Log(LOGWARNING, "CSemanticDatabase::{}: Failed to create vector table", __func__);
  }

  return true;
}
```

### 2. Add VectorSearcher Member to SemanticDatabase

In SemanticDatabase.h:

```cpp
#include "search/VectorSearcher.h"

class CSemanticDatabase : public CDatabase
{
  // ... existing methods ...

  /*!
   * @brief Get the vector searcher instance
   * @return Pointer to vector searcher, or nullptr if not initialized
   */
  CVectorSearcher* GetVectorSearcher() { return m_vectorSearcher.get(); }

private:
  std::unique_ptr<CVectorSearcher> m_vectorSearcher;
};
```

### 3. Usage Pattern

#### Inserting Embeddings

After adding a chunk to the database and generating its embedding:

```cpp
// In embedding pipeline
int64_t chunkId = semanticDb->InsertChunk(chunk);
if (chunkId > 0)
{
  std::array<float, 384> embedding = GenerateEmbedding(chunk.text);

  CVectorSearcher* searcher = semanticDb->GetVectorSearcher();
  if (searcher)
  {
    if (!searcher->InsertVector(chunkId, embedding))
    {
      CLog::Log(LOGERROR, "Failed to insert vector for chunk {}", chunkId);
    }
  }
}
```

#### Searching for Similar Content

To perform semantic search:

```cpp
// Convert user query to embedding
std::string userQuery = "how to configure settings";
std::array<float, 384> queryEmbedding = GenerateEmbedding(userQuery);

// Search for similar vectors
CVectorSearcher* searcher = semanticDb->GetVectorSearcher();
if (searcher)
{
  auto results = searcher->SearchSimilar(queryEmbedding, 50);

  // Retrieve full chunk data for each result
  for (const auto& result : results)
  {
    SemanticChunk chunk;
    if (semanticDb->GetChunk(result.chunkId, chunk))
    {
      // Use chunk data (text, media info, timestamps)
      CLog::Log(LOGINFO, "Found similar content: {} (distance: {})",
                chunk.text, result.distance);
    }
  }
}
```

#### Deleting Vectors

When deleting chunks, also remove their vectors:

```cpp
bool CSemanticDatabase::DeleteChunksForMedia(int mediaId, const MediaType& mediaType)
{
  // Get chunk IDs first
  std::vector<SemanticChunk> chunks;
  GetChunksForMedia(mediaId, mediaType, chunks);

  // Delete vectors
  CVectorSearcher* searcher = GetVectorSearcher();
  if (searcher)
  {
    for (const auto& chunk : chunks)
    {
      searcher->DeleteVector(chunk.chunkId);
    }
  }

  // Delete chunks from database
  return DeleteValues("semantic_chunks",
                      Filter("media_id = " + std::to_string(mediaId)));
}
```

## SQL Schema Integration

The vector table is automatically created by VectorSearcher:

```sql
CREATE VIRTUAL TABLE IF NOT EXISTS semantic_vectors USING vec0(
  chunk_id INTEGER PRIMARY KEY,
  embedding FLOAT[384] distance_metric=cosine
);
```

This table should be documented in SemanticDatabase schema comments but does not need to be created in CreateTables() since VectorSearcher handles it.

## Performance Considerations

### Batch Insertions

For bulk indexing, use transactions:

```cpp
semanticDb->BeginTransaction();

for (const auto& chunk : chunks)
{
  int64_t chunkId = semanticDb->InsertChunk(chunk);
  std::array<float, 384> embedding = GenerateEmbedding(chunk.text);
  searcher->InsertVector(chunkId, embedding);
}

semanticDb->CommitTransaction();
```

### Search Results

The SearchSimilar() method returns results ordered by cosine distance:
- 0.0 = identical vectors
- 1.0 = orthogonal (90° angle)
- 2.0 = opposite vectors

For practical semantic search, consider filtering results with distance < 1.0 for good matches.

## Error Handling

The VectorSearcher logs errors via CLog. Key error conditions:

1. **Extension not initialized**: Check that sqlite-vec source files are present
2. **Table creation failed**: Verify extension loaded successfully
3. **Insert/search failures**: Check vector dimensions match (384)

## Dependencies

- sqlite-vec library (lib/sqlite-vec/)
- SQLite3 with extension support
- Embedding generator producing 384-dimensional vectors

## Future Enhancements

Potential improvements to consider:

1. **Filtered search**: Add WHERE clauses to limit search by media type or date
2. **Index optimization**: Tune sqlite-vec parameters for performance
3. **Hybrid search**: Combine FTS5 full-text search with vector similarity
4. **Batch operations**: Add batch insert/delete methods for efficiency
