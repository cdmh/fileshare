CREATE TABLE `pyfli_files` (
  `id` INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
  `link` TEXT NOT NULL,
  `redirect_url` TEXT DEFAULT NULL,
  `mime_type` TEXT NOT NULL,
  `content_length` INTEGER UNSIGNED NOT NULL,
  `original_filename` TEXT NOT NULL,
  `local_pathname` TEXT NOT NULL,
  `download_count` INTEGER UNSIGNED NOT NULL DEFAULT 0,
  `ts` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
)
ENGINE = InnoDB;

ALTER TABLE `pyfli_files` ADD COLUMN `tweet_screen_name` VARCHAR(15) DEFAULT NULL AFTER `ts`;
ALTER TABLE `pyfli_files` ADD COLUMN `tweet_text` VARCHAR(140) DEFAULT NULL AFTER `tweet_screen_name`;

CREATE TABLE  `pyfli_social_networks` (
  `user_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `network` varchar(20) NOT NULL,
  `oauth_token` text NOT NULL,
  `oauth_token_secret` text NOT NULL,
  `screen_name` text,
  PRIMARY KEY (`user_id`),
  KEY `sn_index_1` (`network`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;