FROM node:alpine AS base

ENV PNPM_HOME="/pnpm"
ENV PATH="$PNPM_HOME:$PATH"

RUN corepack enable

COPY . /app
WORKDIR /app


FROM base AS build

RUN --mount=type=cache,id=pnpm,target=/pnpm/store \
  pnpm install --frozen-lockfile

RUN pnpm run build


FROM base
COPY --from=build /app/.output /app/.output

EXPOSE 3000
CMD [ "node", ".output/server/index.mjs" ]
