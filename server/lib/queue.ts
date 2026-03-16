import type { H3Event } from 'h3'

export interface QueueService {
  /**
   * Send a message to the queue
   * @param message - The message to send
   * @param options - Optional delivery options
   */
  send: (message: unknown, options?: QueueSendOptions) => Promise<void>

  /**
   * Send multiple messages to the queue in a batch
   * @param messages - Array of messages to send
   */
  sendBatch: (messages: unknown[]) => Promise<void>
}

type QueueSendOptions = {
  /**
   * Delay delivery of the message in seconds
   */
  delaySeconds?: number
}

export class CloudflareQueueService implements QueueService {
  private queue: Queue

  constructor(queue: Queue) {
    this.queue = queue
  }

  async send(message: unknown, options?: QueueSendOptions): Promise<void> {
    await this.queue.send(message, {
      delaySeconds: options?.delaySeconds,
    })
  }

  async sendBatch(messages: unknown[]): Promise<void> {
    const batch = messages.map(body => ({ body }))
    await this.queue.sendBatch(batch)
  }
}

export const serverQueue = (event: H3Event): QueueService => {
  const env = event.context.cloudflare.env as unknown as Env

  if (!env.QUEUE) throw new Error('QUEUE binding not found')

  return new CloudflareQueueService(env.QUEUE)
}
