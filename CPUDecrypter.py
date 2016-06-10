from Crypto.Hash import keccak
from Crypto.Cipher import AES
from Crypto.Hash import SHA256
import scrypt
from Crypto.Util import Counter
sha3_256 = lambda x: keccak.new(digest_bits=256, data=x)
import json
import sys
import EncryptedFile

class CPUDecrypter:
    def __init__(self, filepath, password):
        self._enc_file = self._loadFile(filepath)
        self._params = self._get_params()
        self._scrypt_value = self._scrypt_hash(password, *self._params)
        self._cipher_hex = self._decode_hex(self._enc_file.ciphertext)
        self.file_mac = self._decode_hex(self._enc_file.mac)
        self.enc_key = self._encKey()
        self.calculated_mac = self._sha3(self.enc_key)

    def compare(mac1, mac2):
        if mac1 == mac2:
            return True
        else:
            return False

    def _get_params(self):
        return [self._enc_file.salt, self._enc_file.n_value, self._enc_file.r_value, self._enc_file.p_value, self._enc_file.dk_len]

    def _loadFile(self, filepath):
        return EncryptedFile.EncryptedFile(filepath)

    def _decode_hex(self, s):
        if not isinstance(s, (str, unicode)):
            raise TypeError('Value must be an instance of str or unicode')
        return s.decode('hex')

    def _scrypt_hash(self, val, salt, n, r, p, dklen):
        return scrypt.hash(str(val), self._decode_hex(salt), n, r, p, dklen)

    def _encKey(self):
        return self._scrypt_value[16:32] + self._cipher_hex

    def _sha3(self, seed):
        return sha3_256(seed).digest()



if __name__ == "__main__":
    filepath = sys.argv[1]
    password = sys.argv[2]

    cpu_obj = CPUDecrypter(filepath, password)

    keys = cpu_obj.__dict__.keys()
    for i in keys:
        print i, getattr(cpu_obj, i) 
