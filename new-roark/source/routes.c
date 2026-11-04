// routes.c - the stories you can pick on the "Choose your story" screen.
// To add one: write romfs/story/<id>.txt, add a line here, rebuild.
#include "routes.h"

const Route ROUTES[] = {
    {"wendy", "wendy", "Static", "Same school, same war. The whole hallway thinks you stole Suzy's man. Dad's still gone."},
    {"danny", "danny", "Broken Oaths", "You walked away from your family's world. A black car says they want you back."},
    {"peppa", "peppa", "Worthy", "Why does Peppa do the things she does? Baddie Haven Academy is the plan. Baby Evie changes everything."},
    {"pedro", "pedro", "Spots", "Life after Peppa. Danny, Simon and Gerald won't let you mope. Football tryouts are Thursday."},
    {"molly", "molly", "Bookmarked", "Pink gel pen notes on your locker. A boyfriend who's a total green flag. Time to stop squinting."},
    {"suzy", "suzy", "Co-Alpha", "Rebecca's creeping into your spot. Danny swears nothing happened. Mom works doubles."},
    {"gerald", "gerald", "Green Flag", "Molly's boyfriend. The Breakup Club president. Peppa keeps inviting you places."},
    {"freddy", "freddy", "The Others", "Top dog of the Aristocanine Institute. Then a wolf girl at the mall changes how you see the city."},
    {"rebecca", "rebecca", "Number One", "You want to be Peppa's number one. You got caught at the mall. You never quite fit anywhere."},
    {"candy", "candy", "Scrapbook", "After the mall. Therapy at four. Twelve pages of Simon and one empty page labeled Dad."},
    {"karter", "karter", "Both Accents", "Five Aussie cousins are coming. Aunt Karri found Dad's bucket list."},
    {"emily", "emily", "Respect My Name", "Everyone's scapegoat in Peppa's group. Not anymore. And the city sealed Mom's file."},
    {"simon", "simon", "Second Chances", "You cheated on Candy with Rebecca. Everybody knows. Pops says don't run."},
};
const int ROUTE_COUNT = sizeof ROUTES / sizeof ROUTES[0];
