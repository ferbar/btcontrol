#include <stdio.h>

/**
 * fürs wave file lesen
 */
class Reader {
	public:
		Reader(const std::string fileName) {
			this->f=fopen(fileName.c_str(),"r");
			if(!this->f) {
				throw std::runtime_error("error opening " + fileName);
			}

		};
		~Reader() {
			fclose(this->f);
		}
		int32_t ReadInt32() {
			int32_t ret;
			fread(&ret,1,sizeof(ret),this->f);
			return ret;
		}
		int16_t ReadInt16( ) {
			int16_t ret;
			fread(&ret,1,sizeof(ret),this->f);
			return ret;
		}

		int ReadData(int len, std::string &dst) {
			unsigned char *buffer;
			buffer=(unsigned char *) malloc(len);
			if(fread(buffer,1,len,f) != (size_t) len) {
				throw std::runtime_error("error reading file data");
			}
			dst.assign((char*) buffer, len);
			free(buffer);
			return len;
		}

		// void Seek(int skipsize, SeekOrigin::Current ) {
		//  SEEK_SET, SEEK_CUR, or SEEK_END
		void Seek(int skipsize, int a ) {
			abort(); // not yet implemented
		}
	private:
		FILE *f;
};

