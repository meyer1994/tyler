CREATE TABLE "events" (
	"id" text PRIMARY KEY NOT NULL,
	"board_id" text NOT NULL,
	"lat" real NOT NULL,
	"lon" real NOT NULL,
	"alt" real,
	"speed" real,
	"heading" real,
	"hdop" real,
	"sat" integer,
	"ts" timestamp with time zone,
	"point" geometry(point) GENERATED ALWAYS AS (ST_SetSRID(ST_MakePoint("events"."lon", "events"."lat"), 4326)) STORED NOT NULL,
	"created_at" timestamp with time zone DEFAULT CURRENT_TIMESTAMP NOT NULL,
	"updated_at" timestamp with time zone DEFAULT CURRENT_TIMESTAMP NOT NULL
);
--> statement-breakpoint
CREATE TABLE "fences" (
	"id" text PRIMARY KEY NOT NULL,
	"polygon" geometry(Polygon, 4326) NOT NULL,
	"area" real GENERATED ALWAYS AS (ST_Area("fences"."polygon", true)) STORED NOT NULL,
	"created_at" timestamp with time zone DEFAULT CURRENT_TIMESTAMP NOT NULL,
	"updated_at" timestamp with time zone DEFAULT CURRENT_TIMESTAMP NOT NULL
);
--> statement-breakpoint
CREATE TABLE "rel_fences_events" (
	"id" text PRIMARY KEY NOT NULL,
	"fence_id" text,
	"event_id" text,
	"created_at" timestamp with time zone DEFAULT CURRENT_TIMESTAMP NOT NULL,
	"updated_at" timestamp with time zone DEFAULT CURRENT_TIMESTAMP NOT NULL
);
--> statement-breakpoint
CREATE TABLE "account" (
	"id" text PRIMARY KEY NOT NULL,
	"issuer" text NOT NULL,
	"account_id" text NOT NULL,
	"provider_id" text NOT NULL,
	"user_id" text NOT NULL,
	"access_token" text,
	"refresh_token" text,
	"id_token" text,
	"access_token_expires_at" timestamp,
	"refresh_token_expires_at" timestamp,
	"scope" text,
	"password" text,
	"created_at" timestamp DEFAULT now() NOT NULL,
	"updated_at" timestamp NOT NULL
);
--> statement-breakpoint
CREATE TABLE "session" (
	"id" text PRIMARY KEY NOT NULL,
	"expires_at" timestamp NOT NULL,
	"token" text NOT NULL,
	"created_at" timestamp DEFAULT now() NOT NULL,
	"updated_at" timestamp NOT NULL,
	"ip_address" text,
	"user_agent" text,
	"user_id" text NOT NULL,
	CONSTRAINT "session_token_unique" UNIQUE("token")
);
--> statement-breakpoint
CREATE TABLE "user" (
	"id" text PRIMARY KEY NOT NULL,
	"name" text NOT NULL,
	"email" text NOT NULL,
	"email_verified" boolean DEFAULT false NOT NULL,
	"image" text,
	"created_at" timestamp DEFAULT now() NOT NULL,
	"updated_at" timestamp DEFAULT now() NOT NULL,
	CONSTRAINT "user_email_unique" UNIQUE("email")
);
--> statement-breakpoint
CREATE TABLE "verification" (
	"id" text PRIMARY KEY NOT NULL,
	"identifier" text NOT NULL,
	"value" text NOT NULL,
	"expires_at" timestamp NOT NULL,
	"created_at" timestamp DEFAULT now() NOT NULL,
	"updated_at" timestamp DEFAULT now() NOT NULL
);
--> statement-breakpoint
ALTER TABLE "rel_fences_events" ADD CONSTRAINT "rel_fences_events_fence_id_fences_id_fk" FOREIGN KEY ("fence_id") REFERENCES "public"."fences"("id") ON DELETE cascade ON UPDATE cascade;--> statement-breakpoint
ALTER TABLE "rel_fences_events" ADD CONSTRAINT "rel_fences_events_event_id_events_id_fk" FOREIGN KEY ("event_id") REFERENCES "public"."events"("id") ON DELETE cascade ON UPDATE cascade;--> statement-breakpoint
ALTER TABLE "account" ADD CONSTRAINT "account_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "session" ADD CONSTRAINT "session_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
CREATE INDEX "events_board_id_idx" ON "events" USING btree ("board_id");--> statement-breakpoint
CREATE INDEX "events_created_at_idx" ON "events" USING btree ("created_at");--> statement-breakpoint
CREATE INDEX "events_updated_at_idx" ON "events" USING btree ("updated_at");--> statement-breakpoint
CREATE INDEX "events_lat_lon_idx" ON "events" USING btree ("lat","lon");--> statement-breakpoint
CREATE INDEX "events_lon_lat_idx" ON "events" USING btree ("lon","lat");--> statement-breakpoint
CREATE INDEX "events_point_idx" ON "events" USING gist ("point");--> statement-breakpoint
CREATE INDEX "fences_created_at_idx" ON "fences" USING btree ("created_at");--> statement-breakpoint
CREATE INDEX "fences_updated_at_idx" ON "fences" USING btree ("updated_at");--> statement-breakpoint
CREATE INDEX "fences_polygon_idx" ON "fences" USING gist ("polygon");--> statement-breakpoint
CREATE INDEX "fences_events_fence_id_idx" ON "rel_fences_events" USING btree ("fence_id");--> statement-breakpoint
CREATE INDEX "fences_events_event_id_idx" ON "rel_fences_events" USING btree ("event_id");--> statement-breakpoint
CREATE INDEX "fences_events_fence_id_event_id_idx" ON "rel_fences_events" USING btree ("fence_id","event_id");--> statement-breakpoint
CREATE INDEX "fences_events_event_id_fence_id_idx" ON "rel_fences_events" USING btree ("event_id","fence_id");--> statement-breakpoint
CREATE INDEX "fences_events_created_at_idx" ON "rel_fences_events" USING btree ("created_at");--> statement-breakpoint
CREATE INDEX "fences_events_updated_at_idx" ON "rel_fences_events" USING btree ("updated_at");--> statement-breakpoint
CREATE UNIQUE INDEX "account_issuer_accountId_uidx" ON "account" USING btree ("issuer","account_id");--> statement-breakpoint
CREATE INDEX "account_userId_idx" ON "account" USING btree ("user_id");--> statement-breakpoint
CREATE INDEX "session_userId_idx" ON "session" USING btree ("user_id");--> statement-breakpoint
CREATE INDEX "verification_identifier_idx" ON "verification" USING btree ("identifier");