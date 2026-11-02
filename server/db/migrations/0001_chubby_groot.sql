CREATE TABLE "boards" (
	"id" text PRIMARY KEY NOT NULL,
	"name" text,
	"ip" text,
	"last_seen_at" timestamp with time zone,
	"created_at" timestamp with time zone DEFAULT CURRENT_TIMESTAMP NOT NULL,
	"updated_at" timestamp with time zone DEFAULT CURRENT_TIMESTAMP NOT NULL
);
--> statement-breakpoint
CREATE INDEX "boards_last_seen_at_idx" ON "boards" USING btree ("last_seen_at");--> statement-breakpoint
CREATE INDEX "boards_created_at_idx" ON "boards" USING btree ("created_at");--> statement-breakpoint
CREATE INDEX "boards_updated_at_idx" ON "boards" USING btree ("updated_at");