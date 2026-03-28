CREATE TABLE "rel_fences_events" (
	"id" text PRIMARY KEY NOT NULL,
	"fence_id" text NOT NULL,
	"event_id" text NOT NULL,
	"created_at" timestamp with time zone DEFAULT CURRENT_TIMESTAMP NOT NULL,
	"updated_at" timestamp with time zone DEFAULT CURRENT_TIMESTAMP NOT NULL
);
--> statement-breakpoint
ALTER TABLE "rel_fences_events" ADD CONSTRAINT "rel_fences_events_fence_id_fences_id_fk" FOREIGN KEY ("fence_id") REFERENCES "public"."fences"("id") ON DELETE no action ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "rel_fences_events" ADD CONSTRAINT "rel_fences_events_event_id_events_id_fk" FOREIGN KEY ("event_id") REFERENCES "public"."events"("id") ON DELETE no action ON UPDATE no action;--> statement-breakpoint
CREATE INDEX "fences_events_fence_id_idx" ON "rel_fences_events" USING btree ("fence_id");--> statement-breakpoint
CREATE INDEX "fences_events_event_id_idx" ON "rel_fences_events" USING btree ("event_id");--> statement-breakpoint
CREATE INDEX "fences_events_fence_id_event_id_idx" ON "rel_fences_events" USING btree ("fence_id","event_id");--> statement-breakpoint
CREATE INDEX "fences_events_event_id_fence_id_idx" ON "rel_fences_events" USING btree ("event_id","fence_id");--> statement-breakpoint
CREATE INDEX "fences_events_created_at_idx" ON "rel_fences_events" USING btree ("created_at");--> statement-breakpoint
CREATE INDEX "fences_events_updated_at_idx" ON "rel_fences_events" USING btree ("updated_at");--> statement-breakpoint
CREATE INDEX "events_updated_at_idx" ON "events" USING btree ("updated_at");--> statement-breakpoint
CREATE INDEX "events_lon_lat_idx" ON "events" USING btree ("lon","lat");--> statement-breakpoint
CREATE INDEX "fences_created_at_idx" ON "fences" USING btree ("created_at");--> statement-breakpoint
CREATE INDEX "fences_updated_at_idx" ON "fences" USING btree ("updated_at");