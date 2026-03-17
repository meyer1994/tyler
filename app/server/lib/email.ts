import type { H3Event } from 'h3'
import Mailgun from 'mailgun.js'

export const serverEmail = (event: H3Event) => {
  const env = event.context.cloudflare.env as unknown as Env

  const url = env.MAILGUN_API_URL as string | undefined
  const domain = env.MAILGUN_DOMAIN as string | undefined
  const apiKey = env.MAILGUN_API_KEY as string | undefined

  if (!url) throw new Error('MAILGUN_API_URL not found')
  if (!apiKey) throw new Error('MAILGUN_API_KEY not found')
  if (!domain) throw new Error('MAILGUN_DOMAIN not found')

  const mailgun = new Mailgun(FormData)
  const client = mailgun.client({
    url,
    key: apiKey,
    username: 'api',
    // needed to work on cloudflare workers
    // https://github.com/mailgun/mailgun.js/issues/341#issuecomment-3467026163
    useFetch: true,
  })

  type Params = { text: string, to: string }

  const send = async ({ text, to }: Params) => {
    return await client.messages.create(domain, {
      to,
      text,
      subject: 'Login to Flower',
      from: `MagicLink <postmaster@${domain}>`,
    })
  }

  return { send }
}
