# BitArray Unit Tests.
# Originally modified from Peter Cooper's BitField test file.
# http://snippets.dzone.com/posts/show/4234
require "test/unit"
require "bitarray"

class TestLibraryFileName < Test::Unit::TestCase
  def setup
    @public_ba = BitArray.new(1000)
  end
  
  def test_basic
    assert_equal 0, BitArray.new(100)[0]
    assert_equal 0, BitArray.new(100)[1]
  end
  
  def test_setting_and_unsetting
    @public_ba[100] = 1
    assert_equal 1, @public_ba[100]
    @public_ba[100] = 0
    assert_equal 0, @public_ba[100]
  end

  def test_random_setting_and_unsetting
    100.times do
      index = rand(1000)
      @public_ba[index] = 1
      assert_equal 1, @public_ba[index]
      @public_ba[index] = 0
      assert_equal 0, @public_ba[index]
    end
  end
  
  def test_multiple_setting
    1.upto(999) do |pos|
      2.times { @public_ba[pos] = 1 }
      assert_equal 1, @public_ba[pos]
    end
  end

  def test_multiple_unsetting
    1.upto(999) do |pos|
      2.times { @public_ba[pos] = 0 }
      assert_equal 0, @public_ba[pos]
    end
  end
  
  def test_size
    assert_equal 1000, @public_ba.size
  end

  def test_to_s
    ba = BitArray.new(10)
    ba[1] = 1
    ba[5] = 1
    assert_equal "0100010000", ba.to_s
  end

  def test_set_all_bits
    ba = BitArray.new(10)
    ba.set_all_bits
    assert_equal "1111111111", ba.to_s
  end

  def test_clear_all_bits
    ba = BitArray.new(10)
    ba[1] = 1
    ba[5] = 1
    ba.clear_all_bits
    assert_equal "0000000000", ba.to_s
  end

  def test_clone
    ba = BitArray.new(10)
    ba[1] = 1
    ba[5] = 1
    ba_clone = ba.clone
    assert_equal ba_clone.to_s, ba.to_s
  end

  def test_toggle_bit
    ba = BitArray.new(10)
    ba.toggle_bit 5
    assert_equal 1, ba[5] 
  end

  def test_toggle_all_bits
    ba = BitArray.new(10)
    ba[1] = 1
    ba[5] = 1
    ba.toggle_all_bits
    assert_equal "1011101111", ba.to_s
  end

  def test_total_set
    ba = BitArray.new(10)
    ba[1] = 1
    ba[5] = 1
    assert_equal 2, ba.total_set
  end

  def test_slice_beg_len
    ba = BitArray.new(10)
    ba[1] = 1
    ba[5] = 1
    assert_equal "10001", ba[1,5].to_s
    assert_equal "10000", ba[-5,5].to_s
  end

  def test_slice_range
    ba = BitArray.new(10)
    ba[1] = 1
    ba[5] = 1
    assert_equal "10001", ba[1..5].to_s
    assert_equal "10000", ba[-5..-1].to_s
  end

  def test_concatenation
    ba1 = BitArray.new(5)
    ba2 = BitArray.new(5)
    ba2.set_all_bits
    ba3 = ba1 + ba2
    assert_equal "0000011111", ba3.to_s
    ba3 = ba2 + ba1
    assert_equal "1111100000", ba3.to_s
  end

  def test_concatenation2
    ba1 = BitArray.new(32)
    ba2 = BitArray.new(7)
    ba2.set_all_bits
    ba3 = ba1 + ba2
    assert_equal "000000000000000000000000000000001111111", ba3.to_s
    ba3 = ba2 + ba1
    assert_equal "111111100000000000000000000000000000000", ba3.to_s
  end

  def test_to_a
    ba = BitArray.new(16)
    ba[1] = 1
    ba[5] = 1
    assert_equal [0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0], ba.to_a
  end

  def test_init_from_str
    ba = BitArray.new("00011")
    assert_equal "00011", ba.to_s
    ba = BitArray.new("00011abcd")
    assert_equal "00011", ba.to_s
    ba = BitArray.new("abcd0101")
    assert_equal "", ba.to_s
  end


  def test_init_from_array
    ba = BitArray.new([0,1,1,1,0])
    assert_equal "01110", ba.to_s
    ba = BitArray.new([true, true, false, false, true])
    assert_equal "11001", ba.to_s
    ba = BitArray.new([nil, nil, :a, nil, [:b, :c]])
    assert_equal "00101", ba.to_s
  end
end

