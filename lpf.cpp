#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

void write_test_file(const int n, const std::string &filename) {
  std::ofstream writer(filename, std::ios::out | std::ios::binary);
  if (!writer) {
    std::cerr << "Cannot open file!" << std::endl;
    exit(1);
  }

  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution(0.0, 1.0);

  std::vector<double> data(n);
  std::generate(std::begin(data), std::end(data),
                [&]() { return distribution(generator); });
  writer.write(reinterpret_cast<char *>(data.data()),
               data.size() * sizeof(double));
  writer.close();
}

class LowPassFilter {
public:
  LowPassFilter(const int n, const std::vector<double>::iterator &data_block)
      : m_weights(n), m_data_block(data_block) {
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    std::generate(std::begin(m_weights), std::end(m_weights),
                  [&]() { return distribution(generator); });
  }

  double operator()(const int i) const {
    return std::inner_product(std::begin(m_weights), std::end(m_weights),
                              m_data_block + i - m_weights.size(), 0.);
  }

private:
  std::vector<double> m_weights;
  std::vector<double>::iterator m_data_block;
};

int main() {
  const int input_data_size = 1e3;
  const int block_data_size = 1e2;
  const int n_blocks = std::floor(input_data_size / block_data_size);
  const int halo_size = 10;
  const int lpf_size = 10;

  const std::string data_filename = "test_in.dat";
  const std::string processed_data_filename = "test_out.dat";

  // write out some random data for testing
  write_test_file(input_data_size, data_filename);

  // setup reader and writer
  std::ifstream reader(data_filename, std::ios::out | std::ios::binary);
  if (!reader) {
    std::cerr << "Cannot open file!" << std::endl;
    return 1;
  }

  std::ofstream writer(processed_data_filename,
                       std::ios::out | std::ios::binary);
  if (!writer) {
    std::cerr << "Cannot open file!" << std::endl;
    return 1;
  }

  // create some vectors for processing the data
  std::vector<double> halo_and_block(block_data_size + halo_size, 0.);
  std::vector<double> processed_block(block_data_size);
  std::vector<double> indices(block_data_size);
  std::iota(std::begin(indices), std::end(indices), 0);

  // get an iterator to the actual data block (after the halo)
  const std::vector<double>::iterator block =
      std::begin(halo_and_block) + halo_size;

  // create our low pass filter functor
  LowPassFilter lpf(lpf_size, block);

  // read in a new data block at each iteration, process it, then write to the
  // output file
  // we need a "halo" of data at the start of the halo_and_block vector that is at least
  // the size of the low pass filter weights, initially this will be 0, but at the end
  // of each iteration we will fill this "halo" using the last data block
  for (int i = 0; i < n_blocks; i++) {

    // read next block
    reader.read(reinterpret_cast<char *>(&(*block)),
                block_data_size * sizeof(double));

    // perform low pass filter
    std::transform(std::begin(indices), std::end(indices),
                   std::begin(processed_block), lpf);

    // write out processed_block
    writer.write(reinterpret_cast<char *>(processed_block.data()),
                 block_data_size * sizeof(double));

    // put last halo_size elements to the start of the current block, ready for
    // the next block
    std::copy(std::end(halo_and_block) - halo_size, std::end(halo_and_block),
              std::begin(halo_and_block));
  }
  reader.close();
  writer.close();
  return 0;
}
