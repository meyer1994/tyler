CREATE TABLE "fences" (
	"id" text PRIMARY KEY NOT NULL,
	"polygon" geometry(point) NOT NULL,
	"created_at" timestamp with time zone DEFAULT CURRENT_TIMESTAMP NOT NULL,
	"updated_at" timestamp with time zone DEFAULT CURRENT_TIMESTAMP NOT NULL
);
--> statement-breakpoint
CREATE INDEX "fences_polygon_idx" ON "fences" USING gist ("polygon");