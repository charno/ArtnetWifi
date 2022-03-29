/*The MIT License (MIT)

Copyright (c) 2014 Nathanaël Lécaudé
https://github.com/natcl/Artnet, http://forum.pjrc.com/threads/24688-Artnet-to-OctoWS2811

Copyright (c) 2016,2019 Stephan Ruloff
https://github.com/rstephan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <ArtnetWifi.h>
#include <lwip/sockets.h>

const char ArtnetWifi::artnetId[] = ART_NET_ID;

ArtnetWifi::ArtnetWifi() {}

void ArtnetWifi::begin(String hostname)
{
  host = hostname;
  sequence = 1;
  physical = 0;

  // Create socket
  if ((udp_server=socket(AF_INET, SOCK_DGRAM, 0)) == -1){
    log_e("could not create socket: %d", errno);
    return;
  }

  // Bind
  int yes = 1;
  if (setsockopt(udp_server,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0) {
    log_e("could not set socket option: %d", errno);
    if(udp_server != -1)
    {
      close(udp_server);
      udp_server = -1;
    }
    return;
  }

  struct sockaddr_in addr;
  memset((char *) &addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(ART_NET_PORT);
  addr.sin_addr.s_addr = 0x00000000;
  if(bind(udp_server , (struct sockaddr*)&addr, sizeof(addr)) == -1){
    log_e("could not bind socket: %d", errno);
    if(udp_server != -1)
    {
      close(udp_server);
      udp_server = -1;
    }
    return;
  }
  fcntl(udp_server, F_SETFL, O_NONBLOCK);


}

uint16_t ArtnetWifi::read(void)
{
  struct sockaddr_in si_other;
  int slen = sizeof(si_other);

  int len = recvfrom(udp_server, artnetPacket, MAX_BUFFER_ARTNET, MSG_DONTWAIT, (struct sockaddr *) &si_other, (socklen_t *)&slen);


  if (len <= MAX_BUFFER_ARTNET && len > 0)
  {
      senderIp =  IPAddress(si_other.sin_addr.s_addr);

      // Check that packetID is "Art-Net" else ignore
      if (memcmp(artnetPacket, artnetId, sizeof(artnetId)) != 0) {
        return 0;
      }

      opcode = artnetPacket[8] | artnetPacket[9] << 8;

      if (opcode == ART_DMX)
      {
        sequence = artnetPacket[12];
        incomingUniverse = artnetPacket[14] | artnetPacket[15] << 8;
        dmxDataLength = artnetPacket[17] | artnetPacket[16] << 8;

        if (artDmxCallback) (*artDmxCallback)(incomingUniverse, dmxDataLength, sequence, artnetPacket + ART_DMX_START);
        if (artDmxFunc) {
          artDmxFunc(incomingUniverse, dmxDataLength, sequence, artnetPacket + ART_DMX_START);
        }
        return ART_DMX;
      }
      if (opcode == ART_POLL)
      {
        return ART_POLL;
      }
      if (opcode == ART_SYNC)
      {
        return ART_SYNC;
      }
  }

  return 0;
}