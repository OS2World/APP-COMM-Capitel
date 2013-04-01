


                 ////// /////// ////// //  /////// ////// //
                //     //   // //  // //     //   //     //
               //     /////// ////// //     //   ////   //
              //     //   // //     //     //   //     //
             ////// //   // //     //     //   ////// //////


                   ISDN CAPI based Answering Machine
                  and Caller-ID for OS/2, Windows 95,
                  Windows 98, Windows ME, Windows NT,
                      Windows 2000 and Whistler





  Introduction:
  ~~~~~~~~~~~~
   CapiTel is a 32-Bit Answering Machine and Caller-ID for OS/2, Windows 95,
   Windows 98, Windows ME, Windows NT, Windows 2000 and Whistler.
   Graphical and Text-Mode Versions are available for all operating systems.

   Thanks to all the users who sent us their bug reports and feature ideas,
   do not stop with your feedback!

   A special thanks goes to the following people:

     - Kerstin Glodzinski
     - Norbert Schulze
     - Kolja Nowak
     - Peter Franken
     - Dirk Schreiber


  Main Features:
  ~~~~~~~~~~~~~
     - 32-Bit Multi-Threaded Answering Machine (as Shareware)
     - 32-Bit Caller-ID (as FREEWARE)
     - Graphical and Text-Mode versions for OS/2, based on CAPI 1.1 and 2.0
     - Graphical and Text-Mode versions for Win 95/NT, based on CAPI 2.0
     - Complete Support for .WAV-Files
     - Remote Control Functions, controlled by DTMF tones.
     - Silence Detection
     - Completely configurable
     - Callback
     - Year 2000 approved
     - Languages:
       - english
       - german
       - french      (Windows only)
       - spanish     (Windows only)
       - norwegian   (Windows only)
       - nynorsk     (Windows only)
       - dutch       (Windows only)
       - italian     (Windows only)
       - hungarian   (Windows only)
       - finnish     (Windows only)
       - danish      (Windows only)


  Installation under OS/2:
  ~~~~~~~~~~~~~~~~~~~~~~~
   Before you are able to use CapiTel, you have to install the OS/2
   Multimedia Package MMPM/2 and a CAPI 1.1 or CAPI 2.0 compliant
   driver for your ISDN card.

   The installation itself is easy. Just unzip the complete distribution
   archive into the desired destination directory and run INSTALL.CMD.
   This REXX script will create a CapiTel object on your Desktop.

   If you are going to create CapiTel objects on your own, do not
   forget to set the "Working Directory" !

   CAPI 1.1 is unable to handle more than one applications that listen on
   the same EAZ. Therefore: If you use CapiTel, make sure it listens to
   the configured EAZ alone.
   With CAPI 2.0 this is no problem.


  Installation under Win 95/NT:
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Unzip the distribution archive into a temporary directory and
   start SETUP.EXE, this will install CapiTel on your machine and you
   will find it in the Start-Menu.


  Before you begin:
  ~~~~~~~~~~~~~~~~
   This topic is very important, because many users misconfigure CapiTel
   before they even know that it is working at all. Therefore before you
   begin to play with all the properties, please start CapiTel, leave
   the properties alone and call yourself. If CapiTel works on your
   system, you should see an incoming call from your own number. By
   default it listens to all numbers.
   Feel free to configure CapiTel after this little test has been
   successful.

   If you would like to run CapiTel as a FREEWARE Caller-ID, please set up
   ANSWER DELAY to 999 seconds. Then CapiTel is FREE!


  Configuration:
  ~~~~~~~~~~~~~
   Use the Properties-Notebook to configure CapiTel.


   General: - Play Ringing-WAV
              If this item is selected, the Ringing-WAV is played on
              your machine if an incoming call is detected.

            - Generate 16-Bit .WAV Files
              Mark the checkbox if your soundcard supports this.

            - Restore Window on new Calls
              If an incoming call is detected, the CapiTel window pops
              to the front. Under OS/2, it doesn't steal the focus!

            - Confirm on Call-Deletion
              Mark the checkbox if you want to be asked before
              CapiTel deletes a call.

            - Ignore empty Calls
              If you mark this checkbox, all calls with a recording
              time of 0 seconds won't be recognized by CapiTel

            - Expand Area-Code
              If a Caller-ID is detected that can't be found in the
              Callers-Database, the name of the caller's city is shown.

            - DTMF Support
              You can completely disable the DTMF Support (Remote Control)
              with this item.

            - Use ulaw-Codec
              This switch is only important to users in the United States
              of America. It will enable the ulaw-Codec instead of the
              alaw-Codec which is used in Europe.

            - Answer Delay
              Specify the default delay (in seconds) before CapiTel
              answers a call.

            - Max. Recording Time
              Specify the default maximum recording time (in seconds).
              CapiTel automatically hangs up if a caller speaks and
              exceeds this limit!

            - Silence Detection
              Specify the Silence Detection (in seconds).
              CapiTel automaticalls hangs up if a caller stops speaking
              and exceeds this limit!

            - Welcome-WAV
              The default file that is played when CapiTel answers a call.

            - Ringing-WAV
              The default file that is played on your machine when an
              incoming call is detected.

            - Logfile
              CapiTel can log all calls into a log-file. You may also
              specify a Named Pipe or a Device (ie. your printer), if
              your operating system supports this.


   Ports:   To edit, double-click on an entry or use the PopUp-Menu.
            If there is no port listed, CapiTel reacts on ALL available ports!

            Port: A short description for the port.

            EAZ/MSN: The EAZ or the MSN for this port. If you are using
                     a CAPI 1.1 driver, fill in an EAZ between 0 and 9
                     here. If you are using CAPI 2.0, supply a complete
                     MSN.

            Welcome WAV/ALW: The WAV or ALW file that is played to the caller.
                             Use * for the default.

            Ringing WAV: The WAV that is played on your machine when
                         an incoming call is detected.
                         Use * for the default.

            Reaction: CapiTel is able to reject a call. Here you define how it
                      should behave: 'Normal' for normal behaviour, 'Busy' to
                      simulate a busy line, 'Refuse' to refuse the call, and
                      'Unavailable' to simulate an unavailable service.
                      Important: The reaction you define here is only valid
                      for the ISDN card. If another device (ie. an ISDN
                      telephone) is also listening on the EAZ/MSN, the caller
                      won't get CapiTel's reaction!  There is nothing we can
                      do about this. That's the way ISDN works.

            Answer Delay: The answer delay (in seconds) for the port.
                          Use * for the default.

            Max. Recording Time: The maximum recording time (in seconds)
                                 for the port.
                                 Use * for the default.


   Callers: To edit, double-click on an entry or use the PopUp-Menu.
            If someone calls you and the incoming number matches one in this
            container, the caller specific settings are used.

            Name: The caller's name.

            Number: The caller's number which is used when identifying
                    incoming calls. You may use the * wildcard at the end of
                    a number (and only there).. ie. 024192090* matches ALL
                    numbers that start with 024192090.

            The other fields are described under Ports.


   Remote Actions:
            To edit, double-click on an entry or use the PopUp-Menu.

            DTMF Code: If this DTMF Code is detected, the appropiate
                       Action is started.

            Action: The following actions are available:

                    - Remote Control plays all calls that have been
                      recorded. Use the *-Button on your phone to delete
                      them.
                    - Reboot (do not forget that you will get stuck at
                      Win NT's login-prompt when using this feature. You
                      may override the login-prompt using utilities like
                      TweakUI)
                    - Deactivate CapiTel
                    - Quit CapiTel
                    - Execute starts a program on your machine.  You have
                      to fill in the Program, Arguments and Title fields!
                    - Callback. With this DTMF-Code you can define a new
                      callback number. entering no new number disables
                      callback.

            Window Title: This is the window's title of an EXECUTE-Action.


   Use "Save" to save the new settings, or "Cancel" to discard them.

   A note about the Port/Caller specific settings:
   If you specify something for a caller, it takes precedence over the
   settings you might have defined for a Port.


  Usage:
  ~~~~~
   The most important part of the CapiTel main window is the container. There
   you will see all incoming calls. The fields are self explanatory.
   Under OS/2, you may throw colours and fonts into the main window, they
   will be remembered.

   If you are using CapiTel with hidden controls, you can access the various
   menu functions with the popup menu you get when you click with your
   right mouse button on the container. Under OS/2, the window can be moved
   by dragging the sizing border with the right mouse button pressed.

   Use the "Record" option to record a new welcome message by calling
   yourself. A dialog will pop up asking you for a filename. WELCOME.ALW is
   the default. You can use the created ALW files in the Port and
   Callers tabs of the Properties-Notebook.

   Of course you can create a .WAV file yourself and use it directly!

   If someone calls you and wants to stop the Welcome-WAV, he just
   pushes the #-Button on his phone.

   If you have any questions, contact us!


  Hints for Experts:
  ~~~~~~~~~~~~~~~~~
   Almost everything in CapiTel can be configured time-dependent. It can't
   be found in the Properties-Notebook, because we didn't want to confuse
   newbies. Edit CAPITEL.CFG, CAPITEL.PRT, CAPITEL.NAM or CAPITEL.ACT and
   modify the entries as follows:

     entry=<default_value>[~<chunk_1>][~<chunk_2>]...[~<chunk_n>]

     chunk_x=<fromday>-<today>,<fromtime-totime>,<value_for_this_chunk>

   fromday/today can be one of: Mo, Tu, We, Th, Fr, Sa, Su
   Lower Day-Value must be first in the Day-Ranges. ie. Fr-Mo is not allowed!

   fromtime/totime must be xx:yy (xx=hours, yy=minutes) in 24 Hour format.

   CapiTel parses the chunks from left to right and stops after the first hit.
   Time-dependent entries must not exceed 512 characters.
   The default value is used if there is no hit.

   Some examples can be found in CAPITEL.CFG

   If you find bugs regarding this feature, please send us a DETAILED
   bug-report.



  License Agreement:
  ~~~~~~~~~~~~~~~~~
   See LICENSE.TXT or LIZENZ.TXT (German Version) for details. By using
   CapiTel you accept the License Agreement.


  Availability:
  ~~~~~~~~~~~~
   All CapiTel versions can be found here:

     - World Wide Web: http://www.2tec.com

   You are explicitly allowed to distribute the unmodified CapiTel
   ZIP-archive (and the EXE-archive for the Windows version) on the
   Internet, on Shareware CD-Roms, on Bulletin Board Systems and so on.


  Final Note:
  ~~~~~~~~~~
   Comments and Bug-Reports are welcome. But please do not forget that
   CapiTel is an easy-to-use and intuitive answering machine, and not
   a complex voice-mailbox, a fax server or a full CTI system!



   Have fun,
   the authors,


            Werner Fehn            &            Carsten Wimmer

            wf@2tec.com                          cw@2tec.com
          www.2tec.com/wf                      www.2tec.com/cw
        Fax: +49-241-9519041                 Fax: +49-241-9571314

          Schuttenhofweg 220                   Trierer Str. 281a
            52080 Aachen                          52078 Aachen
               Germany                               Germany

