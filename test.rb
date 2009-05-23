# Peter Cooper's BitField test file, modified for BitArray.
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
    assert_equal "[0, 1, 0, 0, 0, 1, 0, 0, 0, 0]", ba.to_s
  end
  
  # This method is not implemented yet.

  #def test_total_set
  #  bf = BitArray.new(10)
  #  bf[1] = 1
  #  bf[5] = 1
  #  assert_equal 2, bf.total_set
  #end
end

