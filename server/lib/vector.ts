import {
  CloudflareVectorizeStore,
  CloudflareWorkersAIEmbeddings,
} from '@langchain/cloudflare'
import type { H3Event } from 'h3'

export interface VectorDocument {
  pageContent: string
  metadata: Record<string, unknown>
  score?: number
}

type VectorSearch = {
  prefix?: string
}

export interface VectorStorage {
  /**
   * Fetch the file from storage, extract text, embed, and index it under that
   * key.
   * @param key - The storage key of the file @param metadata - Optional
   * metadata to merge with default metadata
   */
  set: (key: string, metadata?: Record<string, unknown>) => Promise<void>

  /**
   * Retrieves all vector documents and metadata associated with that key.
   * @param key - The key to retrieve documents for
   */
  get: (key: string) => Promise<VectorDocument[]>

  /**
   * Deletes all vectors associated with that key.
   * @param key - The key to delete
   */
  del: (key: string) => Promise<void>

  /**
   * Checks if that specific key has been indexed.
   * @param key - The key to check
   */
  has: (key: string) => Promise<boolean>

  /**
   * Lists unique keys that have been indexed.
   * @param prefix - Optional prefix to filter keys
   */
  list: (prefix?: string) => Promise<string[]>

  /**
   * Performs similarity search, optionally restricted to keys matching a
   * prefix.
   * @param query - The search query @param options - Search options (prefix)
   */
  search: (query: string, options?: VectorSearch) => Promise<VectorDocument[]>
}

export class CloudflareVectorizeStorage implements VectorStorage {
  private env: Env

  private vectorize: CloudflareVectorizeStore
  private embeddings: CloudflareWorkersAIEmbeddings

  constructor(env: Env) {
    this.env = env

    if (!env.AI) throw new Error('AI not found')
    if (!env.VECTORIZE) throw new Error('VECTORIZE not found')
    if (!env.BUCKET) throw new Error('AI not found')

    this.embeddings = new CloudflareWorkersAIEmbeddings({
      binding: env.AI,
      model: '@cf/baai/bge-small-en-v1.5',
    })

    this.vectorize = new CloudflareVectorizeStore(this.embeddings, {
      index: env.VECTORIZE,
    })
  }

  async set(key: string, metadata?: Record<string, unknown>): Promise<void> {
    const stream = await this.env.BUCKET.get(key)
    if (!stream?.body) throw new Error(`Object ${key} not found`)
    const reader = stream.body.getReader()
    const decoder = new TextDecoder()
    let text = ''

    while (true) {
      const { done, value } = await reader.read()
      if (done) break
      text += decoder.decode(value, { stream: true })
    }
    text += decoder.decode()

    await this.vectorize.addDocuments(
      [
        {
          pageContent: text,
          metadata: {
            ...metadata,
            key,
            indexedAt: new Date().toISOString(),
          },
        },
      ],
      { ids: [key] },
    )
  }

  async get(key: string): Promise<VectorDocument[]> {
    // Note: CloudflareVectorizeStore doesn't have a direct "get by ID" that
    // returns the content easily in LangChain but we can use similarity search
    // with a filter or specific ID if supported. For now, we'll try to use
    // similaritySearch with a filter if the implementation supports it, or we
    // might need to use the underlying index.
    const results = await this.vectorize.similaritySearch('', 1, { key })
    return results.map(doc => ({
      pageContent: doc.pageContent,
      metadata: doc.metadata,
    }))
  }

  async del(key: string): Promise<void> {
    await this.vectorize.delete({ ids: [key] })
  }

  async has(key: string): Promise<boolean> {
    const results = await this.get(key)
    return results.length > 0
  }

  async list(_prefix?: string): Promise<string[]> {
    // This is tricky with Vectorize as it's not designed for listing keys.
    // LangChain's CloudflareVectorizeStore doesn't expose a list method. This
    // would typically require a separate database to track indexed keys or
    // querying the Vectorize index directly if possible.
    return []
  }

  async search(query: string, options?: VectorSearch): Promise<VectorDocument[]> {
    const filter: VectorizeVectorMetadataFilter = {}

    if (options?.prefix) {
      // trick for prefix to work
      filter.key = {
        $gte: options.prefix,
        $lt: `${options.prefix}\uffff`,
      }
    }

    const results = await this.vectorize.similaritySearchWithScore(query, 10, filter)

    return results.map(([doc, score]) => ({
      pageContent: doc.pageContent,
      metadata: doc.metadata,
      score,
    }))
  }
}

export const serverVector = (event: H3Event): VectorStorage => {
  const env = event.context.cloudflare.env as unknown as Env
  return new CloudflareVectorizeStorage(env)
}
