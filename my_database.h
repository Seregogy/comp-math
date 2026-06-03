#pragma once
#include "abstract_database.h"
#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>

struct PairHash {
	size_t operator()(const std::pair<int, int>& p) const {
		return (static_cast<size_t>(p.first) << 32) ^ static_cast<size_t>(p.second);
	}
};

class MyDatabase : public AbstractDatabase {
private:
	std::unordered_map<int, User> users;

	// OwnerId, PostId
	std::unordered_map<std::pair<int, int>, Post, PairHash> posts;

	// посты конкретного автора по дате
	std::unordered_map<int, std::vector<Post*>> wall_posts;

	struct DateComparator {
		bool operator()(const Post* p, int date) const { return p->Date < date; }
		bool operator()(int date, const Post* p) const { return date < p->Date; }
	};

public:
	const User& get_user(int userId) override {
		auto it = users.find(userId);

		if (it != users.end()) {
			return it->second;
		}
		throw DatabaseException("User not found");
	}

	void insert_user(const User& user) override {
		users[user.Id] = user;
	}

	const Post& get_post(int ownerId, int postId) override {
		auto it = posts.find({ ownerId, postId });

		if (it != posts.end()) {
			return it->second;
		}

		throw DatabaseException("Post not found");
	}

	void insert_post(const Post& post) override {
		auto key = std::make_pair(post.OwnerId, post.Id);
		auto it = posts.find(key);

		if (it == posts.end()) {
			posts[key] = post;
			Post* post_ptr = &posts[key];

			auto& wall = wall_posts[post.OwnerId];
			auto bound = std::lower_bound(wall.begin(), wall.end(), post.Date, DateComparator());
			wall.insert(bound, post_ptr);
		}
		else {
			it->second = post;
		}
	}

	void delete_post(int ownerId, int postId) override {
		auto key = std::make_pair(ownerId, postId);
		auto it = posts.find(key);

		if (it != posts.end()) {
			int date = it->second.Date;

			auto& wall = wall_posts[ownerId];
			auto range = std::equal_range(wall.begin(), wall.end(), date, DateComparator());

			for (auto target = range.first; target != range.second; ++target) {
				if ((*target)->Id == postId) {
					wall.erase(target);
					break;
				}
			}
			posts.erase(it);
		}
	}

	void like_post(int ownerId, int postId) override {
		auto it = posts.find({ ownerId, postId });

		if (it != posts.end()) {
			it->second.Likes++;
		}
	}

	void unlike_post(int ownerId, int postId) override {
		auto it = posts.find({ ownerId, postId });

		if (it != posts.end()) {
			it->second.Likes--;
		}
	}

	void repost_post(int ownerId, int postId) override {
		auto it = posts.find({ ownerId, postId });

		if (it != posts.end()) {
			it->second.Reposts++;
		}
	}

	std::vector<Post> top_k_post_by_likes(int k, int ownerId, int dateBegin, int dateEnd) override {
		auto wall_it = wall_posts.find(ownerId);

		if (wall_it == wall_posts.end()) {
			return {};
		}

		const auto& wall = wall_it->second;
		auto start_it = std::lower_bound(wall.begin(), wall.end(), dateBegin, DateComparator());
		auto end_it = std::lower_bound(wall.begin(), wall.end(), dateEnd, DateComparator());

		if (start_it == end_it) {
			return {};
		}

		std::vector<const Post*> sub_wall(start_it, end_it);

		size_t target_size = std::min(static_cast<size_t>(k), sub_wall.size());
		std::partial_sort(sub_wall.begin(), sub_wall.begin() + target_size, sub_wall.end(),
			[](const Post* a, const Post* b) { return a->Likes > b->Likes; });

		std::vector<Post> result;
		result.reserve(target_size);
		for (size_t i = 0; i < target_size; ++i) {
			result.push_back(*sub_wall[i]);
		}

		return result;
	}

	std::vector<Post> top_k_post_by_reposts(int k, int ownerId, int dateBegin, int dateEnd) override {
		auto wall_it = wall_posts.find(ownerId);

		if (wall_it == wall_posts.end()) {
			return {};
		}

		const auto& wall = wall_it->second;
		auto start_it = std::lower_bound(wall.begin(), wall.end(), dateBegin, DateComparator());
		auto end_it = std::lower_bound(wall.begin(), wall.end(), dateEnd, DateComparator());

		if (start_it == end_it) {
			return {};
		}
		std::vector<const Post*> sub_wall(start_it, end_it);

		size_t target_size = std::min(static_cast<size_t>(k), sub_wall.size());
		std::partial_sort(sub_wall.begin(), sub_wall.begin() + target_size, sub_wall.end(),
			[](const Post* a, const Post* b) { return a->Reposts > b->Reposts; });

		std::vector<Post> result;
		result.reserve(target_size);
		for (size_t i = 0; i < target_size; ++i) {
			result.push_back(*sub_wall[i]);
		}
		return result;
	}

	std::vector<UserWithLikes> top_k_authors_by_likes(int k, int ownerId, int dateBegin, int dateEnd) override {
		auto wall_it = wall_posts.find(ownerId);

		if (wall_it == wall_posts.end()) {
			return {};
		}

		const auto& wall = wall_it->second;
		auto start_it = std::lower_bound(wall.begin(), wall.end(), dateBegin, DateComparator());
		auto end_it = std::lower_bound(wall.begin(), wall.end(), dateEnd, DateComparator());

		if (start_it == end_it) {
			return {};
		}

		std::unordered_map<int, int> author_likes_map;
		for (auto it = start_it; it != end_it; ++it) {
			author_likes_map[(*it)->FromId] += (*it)->Likes;
		}

		std::vector<std::pair<int, int>> authors(author_likes_map.begin(), author_likes_map.end());
		size_t target_size = std::min(static_cast<size_t>(k), authors.size());
		std::partial_sort(authors.begin(), authors.begin() + target_size, authors.end(),
			[](const std::pair<int, int>& a, const std::pair<int, int>& b) { return a.second > b.second; });

		std::vector<UserWithLikes> result(target_size);
		for (size_t i = 0; i < target_size; ++i) {
			result[i].User = get_user(authors[i].first);
			result[i].Likes = authors[i].second;
		}

		return result;
	}

	std::vector<UserWithReposts> top_k_authors_by_reports(int k, int ownerId, int dateBegin, int dateEnd) override {
		auto wall_it = wall_posts.find(ownerId);

		if (wall_it == wall_posts.end()) {
			return {};
		}

		const auto& wall = wall_it->second;
		auto start_it = std::lower_bound(wall.begin(), wall.end(), dateBegin, DateComparator());
		auto end_it = std::lower_bound(wall.begin(), wall.end(), dateEnd, DateComparator());

		if (start_it == end_it) {
			return {};
		}

		std::unordered_map<int, int> author_reposts_map;
		for (auto it = start_it; it != end_it; ++it) {
			author_reposts_map[(*it)->FromId] += (*it)->Reposts;
		}

		std::vector<std::pair<int, int>> authors(author_reposts_map.begin(), author_reposts_map.end());
		size_t target_size = std::min(static_cast<size_t>(k), authors.size());
		std::partial_sort(authors.begin(), authors.begin() + target_size, authors.end(),
			[](const std::pair<int, int>& a, const std::pair<int, int>& b) { return a.second > b.second; });

		std::vector<UserWithReposts> result(target_size);
		for (size_t i = 0; i < target_size; ++i) {
			result[i].User = get_user(authors[i].first);
			result[i].Reposts = authors[i].second;
		}

		return result;
	}
};
