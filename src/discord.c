#if __has_include (<discordrpc.h>)
#include <discordrpc.h>

DiscordRPC Discord;
DiscordEventHandlers Handlers;
DiscordActivity Activity;

void InitializeRPC() {
  printf("Initializing Discord RPC.\n");
  memset(&Handlers, 0, sizeof(Handlers));

  DiscordRPC_init(&Discord, "1393325195410014218", &Handlers);

  if (Discord.connected) {
    memset(&Activity, 0, sizeof(Activity));
    Activity.state = "N/A";
    Activity.details = "N/A";
    Activity.startTimestamp = time(NULL);

    DiscordRPC_setActivity(&Discord, &Activity);
  } else {
    printf("Failed to connect to Discord RPC: %s\n", Discord.last_error);
    #ifndef WINDOWS
    printf("Is your Discord running in a flatpak?\n");
    #endif
  }
}

void UpdateActivityRPC(char *Title, char *Artist) {
  Title = Title ? Title : "N/A";
  Artist = Artist ? Artist : "N/A";
  
  Activity.state = Artist;
  Activity.details = Title;

  DiscordRPC_setActivity(&Discord, &Activity);
}

void ShutdownRPC() {
  DiscordRPC_shutdown(&Discord);
}

#else

/* If we opted for no discord rpc, we don't need any functionality. */
void InitializeRPC() {}
void UpdateActivityRPC() {}
void ShutdownRPC() {}

#endif
