// Reading Spotify's player state into NowPlaying of types.ts, and when a change is worth
// sending to the page. The answers are made up, in the shape of GET /v1/me/player.
#include "check.hpp"
#include "spotify.hpp"

using nlohmann::json;

namespace {

json track_player(bool playing = true, int progress = 61000) {
  return {
      {"is_playing", playing},
      {"progress_ms", progress},
      {"currently_playing_type", "track"},
      {"device", {{"name", "Głośnik w salonie"}, {"type", "Speaker"}}},
      {"item",
       {{"type", "track"},
        {"name", "Midnight City"},
        {"duration_ms", 243000},
        {"artists", {{{"name", "M83"}}, {{"name", "Someone Else"}}}},
        {"album",
         {{"name", "Hurry Up, We're Dreaming"},
          {"images",
           {{{"url", "https://i.scdn.co/640"}, {"width", 640}},
            {{"url", "https://i.scdn.co/300"}, {"width", 300}},
            {{"url", "https://i.scdn.co/64"}, {"width", 64}}}}}}}},
  };
}

} // namespace

int main() {
  const json m = dash::now_playing(track_player(), 1'000'000);
  CHECK(m["title"] == "Midnight City");
  CHECK(m["artist"] == "M83, Someone Else");
  CHECK(m["album"] == "Hurry Up, We're Dreaming");
  CHECK(m["artUrl"] == "https://i.scdn.co/640");
  CHECK(m["isPlaying"] == true);
  CHECK(m["progressMs"] == 61000);
  CHECK(m["readAt"] == 1'000'000);
  CHECK(m["durationMs"] == 243000);
  CHECK(m["device"] == "Głośnik w salonie");
  CHECK(m.size() == 9); // the NowPlaying of types.ts

  // Covers: the largest up to 640 px, else the smallest given; none at all is null.
  json big = track_player();
  big["item"]["album"]["images"] = {{{"url", "https://i.scdn.co/1000"}, {"width", 1000}},
                                    {{"url", "https://i.scdn.co/800"}, {"width", 800}}};
  CHECK(dash::now_playing(big, 0)["artUrl"] == "https://i.scdn.co/800");
  big["item"]["album"]["images"] = json::array();
  CHECK(dash::now_playing(big, 0)["artUrl"].is_null());

  // A podcast episode: the show stands in for the artist and the album.
  const json episode_player = {
      {"is_playing", false},
      {"progress_ms", 5000},
      {"device", {{"name", "Telefon"}}},
      {"item",
       {{"type", "episode"},
        {"name", "Odcinek 12"},
        {"duration_ms", 3600000},
        {"images", {{{"url", "https://i.scdn.co/ep"}, {"width", 640}}}},
        {"show", {{"name", "Podcast"}, {"publisher", "Wydawca"}}}}},
  };
  const json e = dash::now_playing(episode_player, 0);
  CHECK(e["title"] == "Odcinek 12");
  CHECK(e["artist"] == "Wydawca");
  CHECK(e["album"] == "Podcast");
  CHECK(e["artUrl"] == "https://i.scdn.co/ep");
  CHECK(e["isPlaying"] == false);

  // An advert or a player without an item: nothing to show.
  CHECK(dash::now_playing({{"is_playing", true}, {"item", nullptr}}, 0).is_null());
  CHECK(dash::now_playing(json::object(), 0).is_null());

  // Publishing: the page moves the bar on by itself, so only real changes go out.
  const json shown = dash::now_playing(track_player(true, 61000), 1'000'000);
  CHECK(!dash::worth_publishing(shown, dash::now_playing(track_player(true, 66000), 1'005'000)));
  CHECK(dash::worth_publishing(shown, dash::now_playing(track_player(true, 120000), 1'005'000))); // a jump
  CHECK(dash::worth_publishing(shown, dash::now_playing(track_player(false, 66000), 1'005'000))); // paused
  json other = track_player();
  other["item"]["name"] = "Wait";
  CHECK(dash::worth_publishing(shown, dash::now_playing(other, 1'005'000)));
  CHECK(dash::worth_publishing(shown, nullptr));
  CHECK(dash::worth_publishing(nullptr, shown));
  CHECK(!dash::worth_publishing(nullptr, nullptr));
  // Paused: the position must stay where it was.
  const json paused = dash::now_playing(track_player(false, 61000), 1'000'000);
  CHECK(!dash::worth_publishing(paused, dash::now_playing(track_player(false, 61000), 1'020'000)));
  return 0;
}
