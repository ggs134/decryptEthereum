

void blkcpy(void * dest, void * src, size_t len)
{
	size_t * D = dest;
	size_t * S = src;
	size_t L = len / sizeof(size_t);
	size_t i;

	for (i = 0; i < L; i++)
		D[i] = S[i];
}

void blkxor(void * dest, void * src, size_t len)
{
	size_t * D = dest;
	size_t * S = src;
	size_t L = len / sizeof(size_t);
	size_t i;

	for (i = 0; i < L; i++)
		D[i] ^= S[i];
}


void salsa20_8(uint32_t B[16])
{
	uint32_t x[16];
	size_t i;

	blkcpy(x, B, 64);
	for (i = 0; i < 8; i += 2) {
#define R(a,b) (((a) << (b)) | ((a) >> (32 - (b))))
		/* Operate on columns. */
		x[4] ^= R(x[0] + x[12], 7);  x[8] ^= R(x[4] + x[0], 9);
		x[12] ^= R(x[8] + x[4], 13);  x[0] ^= R(x[12] + x[8], 18);

		x[9] ^= R(x[5] + x[1], 7);  x[13] ^= R(x[9] + x[5], 9);
		x[1] ^= R(x[13] + x[9], 13);  x[5] ^= R(x[1] + x[13], 18);

		x[14] ^= R(x[10] + x[6], 7);  x[2] ^= R(x[14] + x[10], 9);
		x[6] ^= R(x[2] + x[14], 13);  x[10] ^= R(x[6] + x[2], 18);

		x[3] ^= R(x[15] + x[11], 7);  x[7] ^= R(x[3] + x[15], 9);
		x[11] ^= R(x[7] + x[3], 13);  x[15] ^= R(x[11] + x[7], 18);

		/* Operate on rows. */
		x[1] ^= R(x[0] + x[3], 7);  x[2] ^= R(x[1] + x[0], 9);
		x[3] ^= R(x[2] + x[1], 13);  x[0] ^= R(x[3] + x[2], 18);

		x[6] ^= R(x[5] + x[4], 7);  x[7] ^= R(x[6] + x[5], 9);
		x[4] ^= R(x[7] + x[6], 13);  x[5] ^= R(x[4] + x[7], 18);

		x[11] ^= R(x[10] + x[9], 7);  x[8] ^= R(x[11] + x[10], 9);
		x[9] ^= R(x[8] + x[11], 13);  x[10] ^= R(x[9] + x[8], 18);

		x[12] ^= R(x[15] + x[14], 7);  x[13] ^= R(x[12] + x[15], 9);
		x[14] ^= R(x[13] + x[12], 13);  x[15] ^= R(x[14] + x[13], 18);
#undef R
	}
	for (i = 0; i < 16; i++)
		B[i] += x[i];
}


void blockmix_salsa8(uint32_t * Bin, uint32_t * Bout, uint32_t * X, size_t r)
{
	size_t i;

	/* 1: X <-- B_{2r - 1} */
	blkcpy(X, &Bin[(2 * r - 1) * 16], 64);

	/* 2: for i = 0 to 2r - 1 do */
	for (i = 0; i < 2 * r; i += 2) {
		/* 3: X <-- H(X \xor B_i) */
		blkxor(X, &Bin[i * 16], 64);
		salsa20_8(X);

		/* 4: Y_i <-- X */
		/* 6: B' <-- (Y_0, Y_2 ... Y_{2r-2}, Y_1, Y_3 ... Y_{2r-1}) */
		blkcpy(&Bout[i * 8], X, 64);

		/* 3: X <-- H(X \xor B_i) */
		blkxor(X, &Bin[i * 16 + 16], 64);
		salsa20_8(X);

		/* 4: Y_i <-- X */
		/* 6: B' <-- (Y_0, Y_2 ... Y_{2r-2}, Y_1, Y_3 ... Y_{2r-1}) */
		blkcpy(&Bout[i * 8 + r * 16], X, 64);
	}
}


uint64_t integerify(void * B, size_t r)
{
	uint32_t * X = (void *)((uintptr_t)(B)+(2 * r - 1) * 64);

	return (((uint64_t)(X[1]) << 32) + X[0]);
}


void smix(uint32_t * B, size_t r, uint64_t N, uint32_t * V, uint32_t * XY)
{
	uint32_t * X = XY;
	uint32_t * Y = &XY[32 * r];
	uint32_t * Z = &XY[64 * r];
	uint64_t i;
	uint64_t j;
	size_t k;

	/* 1: X <-- B */
	for (k = 0; k < 32 * r; k++)
		X[k] = le32dec(&B[4 * k]);

	/* 2: for i = 0 to N - 1 do */
	for (i = 0; i < N; i += 2) {
		/* 3: V_i <-- X */
		blkcpy(&V[i * (32 * r)], X, 128 * r);

		/* 4: X <-- H(X) */
		blockmix_salsa8(X, Y, Z, r);

		/* 3: V_i <-- X */
		blkcpy(&V[(i + 1) * (32 * r)], Y, 128 * r);

		/* 4: X <-- H(X) */
		blockmix_salsa8(Y, X, Z, r);
	}

	/* 6: for i = 0 to N - 1 do */
	for (i = 0; i < N; i += 2) {
		/* 7: j <-- Integerify(X) mod N */
		j = integerify(X, r) & (N - 1);

		/* 8: X <-- H(X \xor V_j) */
		blkxor(X, &V[j * (32 * r)], 128 * r);
		blockmix_salsa8(X, Y, Z, r);

		/* 7: j <-- Integerify(X) mod N */
		j = integerify(Y, r) & (N - 1);

		/* 8: X <-- H(X \xor V_j) */
		blkxor(Y, &V[j * (32 * r)], 128 * r);
		blockmix_salsa8(Y, X, Z, r);
	}

	/* 10: B' <-- X */
	for (k = 0; k < 32 * r; k++)
		le32enc(&B[4 * k], X[k]);
}


void crypto_scrypt(uint32_t *pbkdf_result, uint32_t *result)
{

	uint32_t p = 1;
	uint32_t r = 8;
	uint64_t N = 262144;
	uint32_t * B[256] = 0;
	uint32_t * V[32 * r * N] = 0;
	uint32_t * XY[64 * r] = 0;
	uint32_t i;

	for (i = 0; i < 1024; i += 4)
		B[i / 4] = (&pbkdf_result[i + 3] << 24) | (&pbkdf_result[i + 2] << 16) | (&pbkdf_result[i + 1] << 8) | &pbkdf_result[i])
		smix(&B[128 * r], r, N, V, XY);

}

int main(){

	unit32_t

}
