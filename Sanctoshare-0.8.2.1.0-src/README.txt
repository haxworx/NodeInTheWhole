

   Copyright (c) 2015, Al Poole <netstar@gmail.com>
   All rights reserved. 

   USAGE
  
   Run server.js on remote machine with Node.js.
   
   ./sanctoshare <hostname> <watch directory>
 
   e.g.
   
   ./sanctoshare haxlab.org /root/Pictures
  
   Any client instance on the local computer will send files to the server
   which the server will arrange according to folder name. 

   The "watch directory" should be the full path. This stops confusion!

   Major Changes:

   *  As of 0.8.2.1 the IPC has been fixed and improved upon.
      Will resend if broken transfer has occurred upon next run.
      Seems to deal with its children quite well.

   *  Two server.js examples. One with "baked in" username and password,
      one with MySQL query for credentials when doing authentication.
	
   *  As of 0.8.2.0 using some IPC to look after our children connect() and 
      write(). Needs some more work though for in-transfer breakages...

   *  As of 0.8.1.0 we now fix upon breakages in connection/transfers etc.
      see .sanctoshare/process_file usage...
   
   *  As of version 0.8.0.0 our server uses node.js.

   *  As of version 0.8.0.0 we revert to Simplified BSD License.

   *  As of version 0.6.10.0 we do parallel tasks better!

   *  As of version 0.6.8.4 we introduced HTTP/1.1 with SSL.

   THANKS

   Thanks to Sam Watkins, Chris Rahm, Jason Pierce, 
   Ed Poole, Graham Anderson and Mike Parfitt.

   Thanks also be to our God and Father of Our Lord Jesus Christ!

   CONTACT

   Send us an e-mail with your questions, problems, suggestions or to arrange a
   nice sit down, cup of tea and a biscuit!

   E-mail: netstar@gmail.com
   Web: http://haxlab.org

   DONATIONS

   We accept donations of cash, food, beer and computer equipment!

   No seagulls were harmed during the production of SanctoShare.


   SALUTATION

   Cheerio! Cheerio! Cheerio!

