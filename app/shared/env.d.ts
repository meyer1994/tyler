declare module 'h3' {
  interface H3EventContext {
    cf: CfProperties

    cloudflare: {
      env: Env
      request: Request
      context: ExecutionContext
    }
  }
}

export { }
