import json
import sys

class EncryptedFile:
    def __init__(self, filepath):
        self._json = self._get_json(filepath)
        self.ciphertext = self._json["Crypto"]["ciphertext"]
        self.dk_len = self._json["Crypto"]["kdfparams"]["dklen"]
        self.n_value = self._json["Crypto"]["kdfparams"]["n"]
        self.p_value = self._json["Crypto"]["kdfparams"]["p"]
        self.r_value = self._json["Crypto"]["kdfparams"]["r"]
        self.salt = self._json["Crypto"]["kdfparams"]["salt"]
        self.mac = self._json["Crypto"]["mac"]


    def _get_json(self, filepath):
        with open(filepath) as f:
            json_obj = json.loads(f.read())
            return json_obj


if __name__ == "__main__":
    enc_file = EncryptedFile(sys.argv[1])
    print "ciphertext : ", enc_file.ciphertext
    print "dk_len : ", enc_file.dk_len
    print "n_value: ", enc_file.n_value
    print "p_value: ", enc_file.p_value
    print "r_value: ", enc_file.r_value
    print "salt: ", enc_file.salt
    print "mac: ", enc_file.mac
