import type { TRPCContextProtected } from '../lib/trpc'

export const onFileUpload = async (key: string, ctx: TRPCContextProtected) => {
  try {
    const metadata = await ctx.storage.metadata(key)
    await ctx.vector.set(key, { ...metadata, userId: ctx.session.user.id })
  }
  catch (error) {
    console.error(error)
    throw error
  }
}
