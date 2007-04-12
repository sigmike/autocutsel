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
require 'libselsync_test_helper'

class TestSelSync < Test::Unit::TestCase
  include SelSyncTestHelper

  def setup
    @selsync = SelSync.new
  end

  def teardown
    @selsync.free if @selsync
    @server.close if @server and not @server.closed?
    @socket.close if @socket and not @socket.closed?
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
      assert_no_timeout "connecting to #{hostname}" do
        assert server.accept
      end
      server.close
      assert_not_equal 0, @selsync[:socket]
    end
  end
  
  def test_server_accepts_connection
    create_server
    assert_not_equal 0, @selsync[:server], @selsync[:error].to_s
    
    connect_socket
    assert_equal 0, @selsync[:socket]
    @selsync.process_next_event
    assert_not_equal 0, @selsync[:socket]
  end
  
  def test_server_reuse_port
    2.times do |i|
      create_server
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
    request_selection "STRING"
    @selsync.process_next_events
    assert_received request_message
    @socket.write result_message("foo bar")
    @selsync.process_next_events
    wait_for_selection_result
    assert_equal "STRING", @result_type
    assert_equal "foo bar", @result
  end
  
  def test_selection_requested_by_peer
    own_selection_as_string "bob"
    
    create_client_not_owning_selection
    @socket.write request_message
    
    5.times do
      @selsync.process_next_event
    end
    
    assert_received result_message("bob")
  end
  
  def test_client_reconnects_on_connection_lost
    create_client
    @socket.close
    @selsync.process_next_events
    assert_no_timeout "accept" do
      @server.accept
    end
    @selsync.process_next_events
  end
  
  def test_client_owns_selection_on_reconnect
    create_client
    @selsync.disown_selection
    @selsync.process_next_events
    assert_equal 0, @selsync.owning_selection
    @socket.close
    @selsync.process_next_events
    assert_no_timeout "accept" do
      @server.accept
    end
    @selsync.process_next_events
    assert_equal 1, @selsync.owning_selection
  end

  def test_client_reconnects_forever_on_connection_lost
    create_client
    @selsync.set_reconnect_delay 10
    
    3.times do
      @socket.close
      @server.close
      @selsync.process_next_events
      assert_equal 0, @selsync[:socket]
      sleep 0.015
      @server = TCPServer.new @port
      @selsync.process_next_events
      @socket = assert_no_timeout "accept" do
        @server.accept
      end
    end
  end
  
  def test_default_reconnect_delay
    @selsync = SelSync.new
    assert_equal 1000, @selsync[:reconnect_delay]
  end

  def test_server_lost_connection
    create_server
    connect_socket
    @selsync.process_next_events
    old_socket = @selsync[:socket]
    @socket.close
    @selsync.process_next_events
    assert_equal 0, @selsync[:socket]
    connect_socket
    @selsync.process_next_events
    assert_not_equal old_socket, @selsync[:socket]
  end
  
  def test_another_client_connects_to_server
    create_server
    connect_socket
    @selsync.process_next_events
    old_socket = @selsync[:socket]
    socket = TCPSocket.new "localhost", @port
    @selsync.process_next_events
    assert_equal old_socket, @selsync[:socket]
    assert_no_timeout "socket not closed by peer" do
      assert_nil socket.read(1)
    end
  end
  
  def test_request_targets
    create_client_owning_selection
    request_selection "TARGETS"
    @selsync.process_next_events
    wait_for_selection_result
    assert_nothing_received
    assert_equal "ATOM", @result_type
    assert atoms(@result).include?("STRING"), "#{atoms(@result).inspect} doesn't include STRING"
  end
  
  def test_utf8_string_requested
    create_client_owning_selection
    request_selection "UTF8_STRING"
    @selsync.process_next_events
    assert_received request_message
    @socket.write result_message("foo bar")
    @selsync.process_next_events
    wait_for_selection_result
    assert_equal "STRING", @result_type
    assert_equal "foo bar", @result
  end
  
  def test_request_selection_with_property_parameters
  end
end
