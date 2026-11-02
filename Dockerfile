FROM node:22-alpine AS base

ENV PNPM_HOME="/pnpm"
ENV PATH="$PNPM_HOME:$PATH"

RUN corepack enable

COPY . /app
WORKDIR /app

RUN pnpm install --frozen-lockfile
RUN pnpm run build

EXPOSE 3000
CMD [ "sh", "-c", "pnpm run db:migrate && node .output/server/index.mjs" ]
