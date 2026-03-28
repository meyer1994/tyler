ALTER TABLE "rel_fences_events" DROP CONSTRAINT "rel_fences_events_fence_id_fences_id_fk";
--> statement-breakpoint
ALTER TABLE "rel_fences_events" DROP CONSTRAINT "rel_fences_events_event_id_events_id_fk";
--> statement-breakpoint
ALTER TABLE "rel_fences_events" ADD CONSTRAINT "rel_fences_events_fence_id_fences_id_fk" FOREIGN KEY ("fence_id") REFERENCES "public"."fences"("id") ON DELETE cascade ON UPDATE cascade;--> statement-breakpoint
ALTER TABLE "rel_fences_events" ADD CONSTRAINT "rel_fences_events_event_id_events_id_fk" FOREIGN KEY ("event_id") REFERENCES "public"."events"("id") ON DELETE cascade ON UPDATE cascade;