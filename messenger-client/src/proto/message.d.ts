import * as $protobuf from "protobufjs";
import Long = require("long");

/** Namespace messenger. */
export namespace messenger {

    /**
     * Properties of a SecureEnvelope.
     * @deprecated Use messenger.SecureEnvelope.$Properties instead.
     */
    interface ISecureEnvelope extends messenger.SecureEnvelope.$Properties {
    }

    /** Represents a SecureEnvelope. */
    class SecureEnvelope {

        /**
         * Constructs a new SecureEnvelope.
         * @param [properties] Properties to set
         */
        constructor(properties?: messenger.SecureEnvelope.$Properties);

        /** Unknown fields preserved while decoding */
        $unknowns?: Uint8Array[];

        /** SecureEnvelope nonce. */
        nonce: Uint8Array;

        /** SecureEnvelope ciphertext. */
        ciphertext: Uint8Array;

        /** SecureEnvelope senderId. */
        senderId: string;

        /** SecureEnvelope recipientId. */
        recipientId: string;

        /**
         * Creates a new SecureEnvelope instance using the specified properties.
         * @param [properties] Properties to set
         * @returns SecureEnvelope instance
         */
        static create(properties: messenger.SecureEnvelope.$Shape): messenger.SecureEnvelope & messenger.SecureEnvelope.$Shape;
        static create(properties?: messenger.SecureEnvelope.$Properties): messenger.SecureEnvelope;

        /**
         * Encodes the specified SecureEnvelope message. Does not implicitly {@link messenger.SecureEnvelope.verify|verify} messages.
         * @param message SecureEnvelope message or plain object to encode
         * @param [writer] Writer to encode to
         * @returns Writer
         */
        static encode(message: messenger.SecureEnvelope.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

        /**
         * Encodes the specified SecureEnvelope message, length delimited. Does not implicitly {@link messenger.SecureEnvelope.verify|verify} messages.
         * @param message SecureEnvelope message or plain object to encode
         * @param [writer] Writer to encode to
         * @returns Writer
         */
        static encodeDelimited(message: messenger.SecureEnvelope.$Properties, writer?: $protobuf.Writer): $protobuf.Writer;

        /**
         * Decodes a SecureEnvelope message from the specified reader or buffer.
         * @param reader Reader or buffer to decode from
         * @param [length] Message length if known beforehand
         * @returns {messenger.SecureEnvelope & messenger.SecureEnvelope.$Shape} SecureEnvelope
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        static decode(reader: ($protobuf.Reader|Uint8Array), length?: number): messenger.SecureEnvelope & messenger.SecureEnvelope.$Shape;

        /**
         * Decodes a SecureEnvelope message from the specified reader or buffer, length delimited.
         * @param reader Reader or buffer to decode from
         * @returns {messenger.SecureEnvelope & messenger.SecureEnvelope.$Shape} SecureEnvelope
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        static decodeDelimited(reader: ($protobuf.Reader|Uint8Array)): messenger.SecureEnvelope & messenger.SecureEnvelope.$Shape;

        /**
         * Verifies a SecureEnvelope message.
         * @param message Plain object to verify
         * @returns `null` if valid, otherwise the reason why it is not
         */
        static verify(message: { [k: string]: any }): (string|null);

        /**
         * Creates a SecureEnvelope message from a plain object. Also converts values to their respective internal types.
         * @param object Plain object
         * @returns SecureEnvelope
         */
        static fromObject(object: { [k: string]: any }): messenger.SecureEnvelope;

        /**
         * Creates a plain object from a SecureEnvelope message. Also converts values to other types if specified.
         * @param message SecureEnvelope
         * @param [options] Conversion options
         * @returns Plain object
         */
        static toObject(message: messenger.SecureEnvelope, options?: $protobuf.IConversionOptions): { [k: string]: any };

        /**
         * Converts this SecureEnvelope to JSON.
         * @returns JSON object
         */
        toJSON(): { [k: string]: any };

        /**
         * Gets the type url for SecureEnvelope
         * @param [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
         * @returns The type url
         */
        static getTypeUrl(prefix?: string): string;
    }

    namespace SecureEnvelope {

        /** Properties of a SecureEnvelope. */
        interface $Properties {

            /** SecureEnvelope nonce */
            nonce?: (Uint8Array|null);

            /** SecureEnvelope ciphertext */
            ciphertext?: (Uint8Array|null);

            /** SecureEnvelope senderId */
            senderId?: (string|null);

            /** SecureEnvelope recipientId */
            recipientId?: (string|null);

            /** Unknown fields preserved while decoding */
            $unknowns?: Uint8Array[];
        }

        /** Shape of a SecureEnvelope. */
        type $Shape = messenger.SecureEnvelope.$Properties;
    }
}
