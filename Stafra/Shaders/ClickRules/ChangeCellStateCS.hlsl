//                  You really made an entire compute shader just for that?
//
//                                                                                                    
//                                           `````````                                                
//                                      `.--+yhhhhhhhhh////-.``                                       
//                               `-::/+++/::+oyyyyyyyhNs+ooossyo//-`                                  
//                         `-:/++oo:---:++++/+ssyyyyyyNs::+o+:-/oooo+:                                
//                     .:/oo//o+++/+++++/---:/sssssssyNy+o+/:+o+/::+s++.                              
//                  `/o/:://+++:-:/++:-:/+++-+yyyyyyyyNy--:/o+:-:/++oy+o/                             
//                 .o/+++/:---:++/--:++/:----ohhhhhhhhNh+++:-:o+:---:ysso:                            
//                `y+///::/o/:---:++/:-:/o+/-sddddddddNy--:/++//++++::ossh-                           
//                `doo+/++/-:++/:---/++/:-:/-yddddddddNdos/++/+os+++y++syds                           
//                o+:oss+//oo+/+ooo+shsosssy-hmmmmmmmmNh:::/:-::+--:::/:::oo`                         
//               +o.`.-:+sy++o+-::/o+d/-::::-hmmmmmmmmMh-------------------oy`                        
//              :y........-:o+oo....-y------:dNNNNNNNNMh---.----------------o+                        
//             :h-:::////:::::+o.....h------:dNNNNNNNNMh----------.---.------o+                       
//            /y-//:-----::::/:-.-...y/-----:mNNNNNNNNMh---------.-----...----s:                      
//           `h.-.....--.....`...h...-y:.---:mNNNNNNNNMh.---------------------/s                      
//           oh.h///ooo+ooo/:....y:...:y.--:/mNNNNNNNNMd::-------.---------.--/h                      
//           /yyo++oy++++++oyy/-.+-....d/osy/mNNNNNNNNMMNmho/----..-------..../m                      
//            -h--:.:dhysh:++:+++....-+ysso/:hmmmmNNNNMMMMMMNh+-----------....:m                      
//           `y:./s..ddhod:+s-//:-..+so/:--.-/+osyhdmNNMMMMMMMMh:-------....../d                      
//          `y:.-y.../+/+/:........os/--..-:+oyhddmNNNNNMMMMMMMMm/----....-..-h:                      
//         `s:..--................oo:--:+yhmmmNNNNNNNNNMMMMMMMMMMd--------..-y/                       
//         +/`.................../s/-/shdmmNNNNNNNNNNNNNNNMMMMMMMN:------..-y/                        
//        :o.....................so/shhdNNNNNNNNNNNNNNNNNNNNMMMMMm:---....:o.                         
//       -s.....+++sy:..........:s+ohhmNNNNNNNNNNNNNNNNNNNNNNMMMMh-::::--+o`                          
//      `s..........so-........./oohymNNNNNNNNNNNNNNNNNNNNNNNNMMMo:---::h+                            
//      `s:-/o+++//+:.o/........-syyhNNNNNNNNNNNNNNNNNNNNNNNNMMMd.`....+/                             
//        .-.`.-s....../........./dhdNNNNNNNNNNNNNNNNNNNNNNNMMMm-.....:y                              
//             :+-................oddNNNNNNNNNNNNNNNNNNNNNNNMMN:......o-                              
//     ````    +yooo-............-+dmmNmyhhhhdddmmNNNNNNNNMMNh-.......s.                              
//    `/+osdy-`+o+oshy+.......:+shmhs+ymNNNNNNNNmmNmmmNNMMdo-........`y.                              
//     ./sdNdmsy///o/:+:/+oooo+/:-....../oyhmmNNNNNNmdhs+-............s.                              
//        `...`.-+dsss/::-...............-++/.--:my.................../+                              
//             `o/-...................:++/-.....-ms....................y.                             
//             +-.................-/++-..........h+....................-s   ``                        
//             :+.............://+:-.............y/............-/oyhhdddmdddddd-                      
//              -+/:----::://:-/++.............../-........./ohmmNNmmmmmmmmmmmNd-:+:`                 
//                `..-..````     +-.....................:+ydmNmmmmmmNNNNmmNNNNNNmmNmdo.               
//                               +..................-/sdmmNmmmmNNNNNNNNmNNNNNNmmmmmmmNms:.`           
//                               s..............:/shmmNmmmNNNNNNNNNmmmmmmmmNNNNNNNNNNNNNmddhs/-`      
//                            ./so........-:/oydmNNNmNNNNNNNNNmmmNNNNNNNNNNNNNNNNNNNmmmmNNmmdhs-      
//                          -ydmN+:/://oshdmmNNNNNNNNNNNNmmNNNNNNNNNNNNNNmmmmmmmmNNmmdhyo/-..`        
//                         `mNmmmmmNmmNmmmmmmmNNNNNNNNNNNNNNNNNNmmmmmmmNNmmdddhhyo/-.``               
//                        -yNmmmmmmmmmmmmmmmmmmmNNNNNNNNNmmmmmmmmmNNmdhs+:-..``                       
//                       .mNmmmmmmmmmmmmmmmmmmmNNNNNmmmmmmmmNNmdhyo/-.`                               
//                       yNmmmmmmmmmmmmmmmmmmmmmmmmmmmNmmdhy+/-.`                                     
//                      .NmmmmmmmmmmmmmmmmmNNNNmmmddhyo/-.`                                           
//                      .ohhyyyyhhhhhyhyyyyso+//:-.`                                                  
//                                                                                                    
//                                                                                                    
//                                   Yes.                          

cbuffer cbCell: register(b0)
{
	uint gCellX;
	uint gCellY;
};

Texture2D<uint>   gPrevClickRule: register(t0);
RWTexture2D<uint> gNextClickRule: register(u0);

[numthreads(16, 16, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
	if(DTid.x == gCellX && DTid.y == gCellY)
	{
		gNextClickRule[DTid.xy] = (gPrevClickRule[DTid.xy] + 1) % 2;
	}
	else
	{
		gNextClickRule[DTid.xy] = gPrevClickRule[DTid.xy];
	}
}