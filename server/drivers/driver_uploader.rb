require 'drivers/command_packet'

class DriverUploader
  attr_reader :stopped

  module State
    INITIAL = :initial 
    SHOULD_RECEIVE_FILE = :received_file
    FILE_WRITTEN = :completed
  end

  @@state =State::INITIAL

  @@mutex = Mutex.new()
  @@already_sent = false # Global 


  def request_id()
    id = @request_id
    @request_id += 1
    return id
  end

  def _handle_incoming(command_packet)
    if(Settings::GLOBAL.get("packet_trace"))
      window = _get_pcap_window()
      window.puts("IN:  #{command_packet}")
    end

    # if(command_packet.get(:is_request))
    #   tunnel_data_incoming(command_packet)
    #   return
    # end

    handler = @handlers.delete(command_packet.get(:request_id))
    if(handler.nil?)
      @window.puts("Received a response that we have no record of sending:")
      @window.puts("#{command_packet}")
      @window.puts()
      @window.puts("Here are the responses we're waiting for:")
      @handlers.each_pair do |request_id, the_handler|
        @window.puts("#{request_id}: #{the_handler[:request]}")
      end
      return
    end

    if(command_packet.get(:command_id) == CommandPacket::COMMAND_ERROR)
      @window.puts("Client returned an error: #{command_packet.get(:status)} :: #{command_packet.get(:reason)}")
      return
    end

    if(handler[:request].get(:command_id) != command_packet.get(:command_id))
      @window.puts("Received a response of a different packet type (that's really weird, please report if you can reproduce!")
      @window.puts("#{command_packet}")
      @window.puts()
      @window.puts("The original packet was:")
      @window.puts("#{handler}")
      return
    end

    handler[:proc].call(handler[:request], command_packet)
  end

  def _send_request(request, callback)
    # Make sure this is synchronous so threads don't fight
    @@mutex.synchronize() do
      if(callback)
        @handlers[request.get(:request_id)] = {
          :request => request,
          :proc => callback
        }
      end

      if(Settings::GLOBAL.get("packet_trace"))
        window = _get_pcap_window()
        window.puts("OUT: #{request}")
      end

      out = request.serialize()
      out = [out.length, out].pack("Na*")
      @outgoing += out
    end
  end
  
  def initialize(window, settings)
    @window = window
    @settings = settings
    @outgoing = ""
    @incoming = ""
    @request_id = 0x0001
    @handlers = {}
    @stopped = false

    @window.on_input() do |data|
      @outgoing += data
      @outgoing += "\n"
    end

    @window.puts("This is an uploader session!")
    @window.puts()

    @window.puts("To go back, type ctrl-z.")
    @window.puts()
  end

  def feed(data)
    if @@state == State::INITIAL
      # In the first state should send the download command 
      remote_file = data
      base_name = File.basename(remote_file)

      # Join the filename with the DNSCAT_UPLOAD_FOLDER environment variable
      local_file = File.join(ENV['DNSCAT_UPLOAD_FOLDER'], base_name)

      # Now update the state, should be done once 
      @@state = State::SHOULD_RECEIVE_FILE
      
      download = CommandPacket.new({
        :is_request => true,
        :request_id => request_id(),
        :command_id => CommandPacket::COMMAND_DOWNLOAD,
        :filename => remote_file,
      })
      
      puts "Send request download"
      _send_request(download, Proc.new() do |request, response|
        puts "Write the file"
        File.open(local_file, "wb") do |f|
          f.write(response.get(:data))
          @window.puts("Wrote #{response.get(:data).length} bytes from #{request.get(:filename)} to #{local_file}!")
          # File received and written
          @@state = State::FILE_WRITTEN
        end



      end)
  
      
      # out = @outgoing
      # @outgoing = ''
      # return out
    

    elsif @@state == State::SHOULD_RECEIVE_FILE
      # Handling the second behavior: file received
      @incoming += data
      loop do
        if @incoming.length < 4
          break
        end
  
        # Try to read a length + packet
        length, data = @incoming.unpack("Na*")
  
        # If there isn't enough data, give up
        if data.length < length
          break
        end
  
        # Otherwise, remove what we have from @data
        length, data, @incoming = @incoming.unpack("Na#{length}a*")
        _handle_incoming(CommandPacket.parse(data))
      end
  
    elsif @@state == State::FILE_WRITTEN
        # Now send shutdown
        puts "Now shutdown the session"
        shutdown = CommandPacket.new({
          :is_request => true,
          :request_id => request_id(),
          :command_id => CommandPacket::COMMAND_SHUTDOWN,
        })
        _send_request(shutdown, Proc.new() do |request, response|
        end)
        puts "In shutdown _send_request()"
        @window.puts("Shutdown response received")
        @window.puts("Attempting to shut down remote session(s)...")
        @window.close()
        @@state = State::INITIAL
      end

      # Return the queue and clear it out
      result = @outgoing
      @outgoing = ''
      return result
  end
  
  def shutdown()
  end

end
