//
// ファイルを難読化
//

#include <string>
#include <fstream>
#include <cassert>
#include <zlib.h>


namespace ngs { namespace TextCodec {

enum {
	OUTBUFSIZ = 1024 * 8,
};


// 圧縮
std::string encode(const std::string& input) noexcept {
	z_stream z;
	z.zalloc = Z_NULL;
	z.zfree	 = Z_NULL;
	z.opaque = Z_NULL;
	deflateInit(&z, Z_DEFAULT_COMPRESSION);

  // SOURCE:http://yak-ex.blogspot.jp/2012/12/c-advent-calendar-2012-8-c-compiler-farm.html
	z.next_in  = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input.c_str()));
	z.avail_in = static_cast<unsigned int>(input.size());

	Bytef outbuf[OUTBUFSIZ];
	z.next_out  = outbuf;
	z.avail_out = OUTBUFSIZ;

  std::string output;
	while (1) {
		int status = deflate(&z, Z_FINISH);
    assert(status != Z_STREAM_ERROR);
    
		if ((z.avail_out == 0) || (status == Z_STREAM_END)) {
      u_int count = OUTBUFSIZ - z.avail_out;
			output.insert(output.end(), &outbuf[0], &outbuf[count]);

      if (status == Z_STREAM_END) break;
      
			z.next_out  = outbuf;
			z.avail_out = OUTBUFSIZ;
		}
	}
	deflateEnd(&z);

  return output;
}


// 書き出し
void write(const std::string& path, const std::string& input) noexcept {
	auto output = encode(input);

	std::ofstream fstr(path, std::ios::binary);
  assert(fstr);

  // SOURCE: http://blogs.wankuma.com/episteme/archive/2009/01/09/166002.aspx
  std::copy(output.begin(), output.end(), std::ostreambuf_iterator<char>(fstr));
}

// 読み込み
std::string load(const std::string& path) noexcept {
	std::ifstream fstr(path, std::ios::binary);
  assert(fstr);

  // SOURCE: http://fa11enprince.hatenablog.com/entry/2014/04/03/233500
  std::string input((std::istreambuf_iterator<char>(fstr)),
                    std::istreambuf_iterator<char>());

  return input;
}

} }


void printHelp() {
	printf("Text file to dz\n");
	printf("Usage:filedz input output\n");
}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printHelp();
		return 0;
	}

  auto text = ngs::TextCodec::load(argv[1]);
  ngs::TextCodec::write(argv[2], text);
}
