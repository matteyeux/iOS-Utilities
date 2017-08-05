all: pbzx imagine ota

pbzx:
		@echo 'Building pbzx...'
		@$(CC) pbzx.c -o pbzx
		@echo 'Successfully built pbzx'

imagine:
		@echo 'Building imagine...'
		@$(CC) imagine.c -o imagine -w
		@echo 'Successfully built imagine'
ota:
		@echo 'Building ota...'
		@$(CC) ota.c -o ota
		@echo 'Successfully built ota'

clean:
		@echo 'Cleaning...'
		@rm -f imagine ota pbzx
		@echo 'Cleaned'
install:
		@echo 'Installing iOS Utilities'
		@cp ota imagine pbzx /usr/local/bin/
		@echo 'Installed iOS Utilities'
