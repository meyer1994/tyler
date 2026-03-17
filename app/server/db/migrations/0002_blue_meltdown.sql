CREATE INDEX `items_user_id_idx` ON `items` (`user_id`);--> statement-breakpoint
CREATE INDEX `items_created_at_idx` ON `items` ("created_at" desc);--> statement-breakpoint
CREATE INDEX `items_updated_at_idx` ON `items` ("updated_at" desc);--> statement-breakpoint
CREATE INDEX `tasks_status_idx` ON `tasks` (`status`);--> statement-breakpoint
CREATE INDEX `tasks_user_id_idx` ON `tasks` (`user_id`);--> statement-breakpoint
CREATE INDEX `tasks_created_at_idx` ON `tasks` ("created_at" desc);--> statement-breakpoint
CREATE INDEX `tasks_updated_at_idx` ON `tasks` ("updated_at" desc);