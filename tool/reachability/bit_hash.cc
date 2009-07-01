
#include "bit_hash.hh"

bit_hash_t::bit_hash_t(size_t s) {
  ht_size = s;
  start_ptr = new char[ht_size];
  memset(start_ptr,0,ht_size);
  collisions = 0;
  hf_id = 0;
}

size_t bit_hash_t::hash_function(state_t q) {
// vraci hodnotu z intervalu 0..8*ht_size-1
  size_t result = 19;

  if (! q.ptr) { cerr <<"Hash function called for empty state."; return 0; }                     

  unsigned char *tmp_ptr = reinterpret_cast<unsigned char*>(q.ptr);

  switch(hf_id) {
  case 0:
    for(size_t tmp=0;tmp<q.size;tmp++) {
	if (*tmp_ptr==0)  result *= 99277;
	else {
	  result += 104729 ^ (*tmp_ptr) ;
	  result *= 57193;
	}
	tmp_ptr ++;
      }
    result %= (8*ht_size);
    break;
  default:
    cerr<<"Unknown hash function."<<endl;
    break;
  }
  //  cout << "hash value: "<<result<<endl;
  return (result);  
}

bool bit_hash_t::check(state_t q) {

  size_t address = hash_function(q);
  char c = start_ptr[address/8];
  size_t bit = address % 8;
  if (c & (1<<bit)) {
    collisions++;
    //    cout << "colision at "<< address/8<<" "<<bit<<endl;
    return true;
  }
  start_ptr[address/8] = start_ptr[address/8] | (1<<bit);
  //  cout << "new one at "<< address/8<<" "<<bit<<endl;
  return false;
}

void bit_hash_t::print() {
  cout << "***********HT************"<<endl;
  for(size_t i=0; i<ht_size; i++) {
    cout << i<<": ";
    char c = start_ptr[i];
    for (int j=0; j<8; j++) {
      cout << ((c & (1<<j))?1:0);
    }
    cout <<endl;
  }
  cout << "*************************"<<endl<<endl;
}
