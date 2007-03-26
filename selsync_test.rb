=begin
 * selsync by Michael Witrant <mike @ lepton . fr>
 * Copyright (c) 2001-2007 Michael Witrant.
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

module SelSync
  extend DL::Importable
  dlload ".libs/libselsync.so"
  extern "struct selsync *selsync_init()"
  extern "int selsync_parse_arguments(struct selsync *, int, char **)"
  extern "void selsync_free(struct selsync *)"
  extern "int selsync_valid(struct selsync *)"
  
  module_function
  def method_missing(method, *args, &block)
    name = "selsync_#{method}"
    if respond_to? name
      send(name, *args, &block)
    else
      raise "Function #{method} or #{name} not imported in #{inspect}"
    end
  end
  
  def new
    selsync = init
    def selsync.method_missing(name, *args, &block)
      fullname = "selsync_#{name}"
      if SelSync.respond_to?(fullname)
        result = SelSync.send(fullname, self, *args, &block)
        struct! 'IISI', :check, :client, :hostname, :port
        result
      else
        raise "No method #{name} on #{inspect} nor #{fullname} on #{SelSync.inspect}"
      end
    end
    selsync
  end
end

  
class TestSelSync < Test::Unit::TestCase
  def test_init
    selsync = SelSync.init
    assert selsync
    selsync.struct! 'ISI', :check, :hostname, :port
    assert_equal 1, SelSync.valid(selsync)
  end
  
  def test_free
    selsync = SelSync.init
    SelSync.free(selsync)
    assert_equal 0, SelSync.valid(selsync)
  end
  
  def test_parse_client_arguments
    selsync = SelSync.new
    assert_equal 1, selsync.parse_arguments(3, ["./selsync", "bob", "4567"])
    assert_equal 1, selsync[:client]
    assert_equal "bob", selsync[:hostname].to_s
    assert_equal 4567, selsync[:port]
  end

  def test_parse_server_arguments
    selsync = SelSync.new
    assert_equal 1, selsync.parse_arguments(2, ["./selsync", "778"])
    assert_equal 0, selsync[:client]
    assert_equal nil, selsync[:hostname]
    assert_equal 778, selsync[:port]
  end
  
  def test_parse_wrong_arguments
    [
      ["./selsync"],
      [],
      ["foo", "bar", "baz", "bob"],
    ].each do |args|
      selsync = SelSync.new
      assert_equal 0, selsync.parse_arguments(args.size, args.empty? ? nil : args), args.inspect
    end
  end
end
