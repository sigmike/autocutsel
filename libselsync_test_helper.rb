=begin
 * selsync by Michael Witrant <mike @ lepton . fr>
 * Copyright (c) 2007 Michael Witrant.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This program is distributed under the terms
 * of the GNU General Public License (read the COPYING file)
 * 
=end

class SelSync
  module LIB
    extend DL::Importable
    dlload ".libs/libselsync.so"
    
    extern "struct selsync *selsync_init()"
    extern "int selsync_parse_arguments(struct selsync *, int, char **)"
    extern "void selsync_free(struct selsync *)"
    extern "int selsync_valid(struct selsync *)"
    extern "void selsync_start(struct selsync *)"
    extern "void selsync_process_next_event(struct selsync *)"
    extern "void selsync_process_next_events(struct selsync *)"
    extern "int selsync_owning_selection(struct selsync *)"
    extern "void selsync_disown_selection(struct selsync *)"
    extern "void selsync_set_socket(struct selsync *, int)"
    extern "int selsync_own_selection(struct selsync *)"
    extern "void selsync_set_debug(struct selsync *, int)"
    extern "void selsync_set_reconnect_delay(struct selsync *, int)"
    module_function
    
  end

  def initialize
    @data = LIB.selsync_init
    @struct_info = [""]
    [
      ['I', :magic],
      ['I', :client],
      ['S', :hostname],
      ['I', :port],
      ['I', :socket],
      ['I', :server],
      ['S', :error],
      ['I', :widget],
      ['I', :selection],
      ['I', :debug],
      ['P', :selection_event],
      ['I', :input_id],
      ['I', :reconnect_delay],
    ].each do |type, name|
      @struct_info.first << type
      @struct_info << name
    end
    parse_data
  end
  
  def [](name)
    @data[name]
  end
  
  def parse_data
    @data.struct! *@struct_info
  end
  
  def method_missing(name, *args, &block)
    fullname = "selsync_#{name}"
    if LIB.respond_to?(fullname)
      result = LIB.send(fullname, @data, *args, &block)
      parse_data
      result
    else
      raise "No method #{name} on #{inspect} nor #{fullname} on #{LIB.inspect}"
    end
  end
end

module SelSyncTestHelper
  def lost_message
    [6, 2].pack('cc')
  end
  
  def request_message
    [6, 0].pack('cc')
  end
  
  def result_message content
    [6, 1, content.size, content].pack('ccia*')
  end
  
  def assert_no_timeout msg = nil, delay = 1
    begin
      timeout delay do
        yield
      end
    rescue Timeout::Error
      assert false, "execution timed out. #{msg}"
    end
  end
  
  def create_server
    @port = 8859
    @selsync = SelSync.new
    @selsync.parse_arguments(2, ["./selsync", @port.to_s])
    @selsync.start
  end
  
  def connect_socket
    assert_nothing_raised do
      timeout 1 do
        @socket = TCPSocket.new "localhost", @port
      end
    end
  end

  def assert_received message, msg = nil
    assert_nothing_raised "while waiting for #{message.inspect}. #{msg}" do
      timeout 1 do
        assert_equal message, @socket.read(message.size), msg
      end
    end
  end
  
  def assert_nothing_received
    assert_raises Timeout::Error, "message received on socket" do
      timeout 0.1 do
        @socket.read(1)
      end
    end
  end
  
  def create_client_with_socket
    @port = 4567
    server = TCPServer.new @port
    @selsync_socket = TCPSocket.new 'localhost', @port
    @socket = server.accept
    server.close
    @selsync = SelSync.new
    @selsync.set_socket @selsync_socket.fileno
    @selsync.start
  end
  
  def create_client_owning_selection
    create_client_with_socket
    @selsync.own_selection
  end
  
  def create_client_not_owning_selection
    create_client_with_socket
  end
  
  def create_client
    @port = 4568
    @server = TCPServer.new @port
    @selsync = SelSync.new
    assert_equal 0, @selsync.owning_selection
    @selsync.parse_arguments(3, ["./selsync", "localhost", @port.to_s])
    @selsync.start
    @socket = @server.accept
  end
  
  def request_selection target
    @pid = fork do
      exec "./cutsel -s PRIMARY request_selection #{target} >test_result"
    end
    sleep 0.2
  end
  
  def wait_for_selection_result
    assert_no_timeout 1 do
      Process.waitpid @pid
    end
    @result_type, @result = File.read("test_result").split("\n", 2)
    @result_type = @result_type.scan(/Type: (.+)/).first.first
  end
  
  def atoms data
    data.split("\n")
  end
  
  def own_selection string
    pid = fork do
      exec "./cutsel -s PRIMARY utf8 #{string}"
    end
    sleep 0.1
  end
end
