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

require 'test/unit'
require 'dl/import'
require 'socket'
require 'timeout'

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

class TestSelSync < Test::Unit::TestCase
  def setup
    @selsync = SelSync.new
  end

  def teardown
    @selsync.free if @selsync
  end

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
  
  def test_init
    @selsync = SelSync.new
    assert @selsync
    assert_equal 1, @selsync.valid
    assert_equal 0, @selsync[:client]
    assert_equal nil, @selsync[:hostname]
    assert_equal 0, @selsync[:port]
    assert_equal 0, @selsync[:socket]
    assert_equal 0, @selsync[:server]
    assert_not_equal 0, @selsync[:widget]
    assert_not_equal 0, @selsync[:selection]
  end
  
  def test_free
    selsync = SelSync.new
    selsync.free
    assert_equal 0, selsync.valid
  end
  
  def test_parse_client_arguments
    @selsync = SelSync.new
    assert_equal 1, @selsync.parse_arguments(3, ["./selsync", "bob", "4567"])
    assert_equal 1, @selsync[:client]
    assert_equal "bob", @selsync[:hostname].to_s
    assert_equal 4567, @selsync[:port]
  end

  def test_parse_server_arguments
    @selsync = SelSync.new
    assert_equal 1, @selsync.parse_arguments(2, ["./selsync", "778"])
    assert_equal 0, @selsync[:client]
    assert_equal nil, @selsync[:hostname]
    assert_equal 778, @selsync[:port]
  end
  
  def test_parse_wrong_arguments
    [
      ["./selsync"],
      ["foo", "bar", "baz", "bob"],
    ].each do |args|
      @selsync = SelSync.new
      assert_equal 0, @selsync.parse_arguments(args.size, args), args.inspect
    end
  end
  
  def test_client_connects
    %w( 127.0.0.1 localhost ).each do |hostname|
      server = TCPServer.new 4567
      @selsync = SelSync.new
      @selsync.parse_arguments(3, ["./selsync", hostname, "4567"])
      @selsync.start
      assert_nothing_raised "connecting to #{hostname}" do
        timeout 1 do
          assert server.accept
        end
      end
      server.close
      assert_not_equal 0, @selsync[:socket]
    end
  end
  
  def test_server_accepts_connection
    @selsync = SelSync.new
    @selsync.parse_arguments(2, ["./selsync", "8859"])
    @selsync.start
    assert_not_equal 0, @selsync[:server], @selsync[:error].to_s
    
    assert_nothing_raised do
      timeout 1 do
        socket = TCPSocket.new "localhost", 8859
      end
    end
    assert_equal 0, @selsync[:socket]
    @selsync.process_next_event
    assert_not_equal 0, @selsync[:socket]
  end
  
  def test_server_reuse_port
    2.times do |i|
      @selsync = SelSync.new
      @selsync.parse_arguments(2, ["./selsync", "8857"])
      @selsync.start
      assert_not_equal 0, @selsync[:server], "pass #{i}: #{@selsync[:error]}"
      @selsync.free
      @selsync = nil
    end
  end
  
  def test_client_owns_selection_on_start
    server = TCPServer.new 45699
    @selsync = SelSync.new
    assert_equal 0, @selsync.owning_selection
    @selsync.parse_arguments(3, ["./selsync", "localhost", "45699"])
    @selsync.start
    assert_equal 1, @selsync.owning_selection
  end
  
  def assert_received message, msg = nil
    assert_nothing_raised "while waiting for #{message.inspect}. #{msg}" do
      timeout 1 do
        assert_equal message, @socket.read(message.size), msg
      end
    end
  end
  
  def create_client_with_socket
    server = TCPServer.new 4567
    @selsync_socket = TCPSocket.new 'localhost', 4567
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
  
  def test_client_lost_selection
    create_client_owning_selection
    @selsync.disown_selection
    @selsync.process_next_events
    assert_equal 0, @selsync.owning_selection
    assert_received lost_message
  end
  
  def test_lost_message_received_from_peer
    create_client_not_owning_selection
    @socket.write lost_message
    @selsync.process_next_event
    assert_equal 1, @selsync.owning_selection
  end
  
  def test_selection_requested_by_an_application
    create_client_owning_selection
    pid = fork do
      exec "./cutsel -s PRIMARY sel >test_result"
    end
    sleep 0.1
    @selsync.process_next_events
    assert_received request_message
    @socket.write result_message("foo bar")
    @selsync.process_next_events
    timeout 10 do
      Process.waitpid pid
    end
    assert_equal "foo bar\n", File.read("test_result")
  end
  
  def test_selection_requested_by_peer
    pid = fork do
      exec "./cutsel -s PRIMARY sel bob"
    end
    sleep 0.1
    
    create_client_not_owning_selection
    @socket.write request_message
    
    5.times do
      @selsync.process_next_event
    end
    
    assert_received result_message("bob")
  end
  
  def test_selection_value_received
  end
  
  def test_client_reconnects_on_connection_lost
    server = TCPServer.new 4568
    @selsync = SelSync.new
    assert_equal 0, @selsync.owning_selection
    @selsync.parse_arguments(3, ["./selsync", "localhost", "4568"])
    @selsync.start
    socket = server.accept
    socket.close
    @selsync.process_next_events
    assert_no_timeout "accept" do
      server.accept
    end
  end

  def test_client_reconnects_forever_on_connection_lost
    server = TCPServer.new 41688
    @selsync = SelSync.new
    assert_equal 0, @selsync.owning_selection
    @selsync.parse_arguments(3, ["./selsync", "localhost", "41688"])
    @selsync.start
    socket = server.accept
    
    @selsync.set_reconnect_delay 10
    
    3.times do
      socket.close
      server.close
      @selsync.process_next_events
      sleep 0.015
      server = TCPServer.new 41688
      @selsync.process_next_events
      socket = assert_no_timeout "accept" do
        server.accept
      end
    end
  end
  
  def test_default_reconnect_delay
    @selsync = SelSync.new
    assert_equal 500, @selsync[:reconnect_delay]
  end

  def test_server_lost_connection
  end
  
  def test_another_client_connects_to_server
  end
end