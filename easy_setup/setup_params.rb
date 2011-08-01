require 'java' # jruby
require 'win32/registry'

    # returns blank if none set...
    def get_user_input(message, default = '', cancel_ok = false)
      received = javax.swing.JOptionPane.showInputDialog(message, default)
      received
    end
    
    screen_capture = Win32::Registry::HKEY_CURRENT_USER.create "Software\\os_screen_capture" # LODO .keys fails?

    for type in ['height', 'width', 'start_x', 'start_y']
      received = get_user_input('enter desired ' + type + ' (blank for reset to default)', screen_capture[type])
      raise 'canceled' unless received
      p received
      if received == ''
        received = 0 # reset...
      else
        received = received.to_i
      end
      if received % 2 == 1 # it's odd
        p "warning--odd numbers are rejected from mplayer for some reason!"
      end
      screen_capture.write(type, Win32::Registry::REG_DWORD, received.to_i)
    end
    screen_capture.close
    p 'done setting them'