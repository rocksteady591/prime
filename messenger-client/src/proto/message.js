/*eslint-disable block-scoped-var, id-length, no-control-regex, no-magic-numbers, no-mixed-operators, no-prototype-builtins, no-redeclare, no-shadow, no-var, sort-vars, default-case, jsdoc/require-param*/
import $protobuf from "protobufjs/minimal.js";

// Common aliases
const $Reader = $protobuf.Reader, $Writer = $protobuf.Writer, $util = $protobuf.util;
const $Object = $util.global.Object, $undefined = $util.global.undefined, $Error = $util.global.Error, $TypeError = $util.global.TypeError, $String = $util.global.String, $Array = $util.global.Array;

// Exported root namespace
const $root = $protobuf.roots["default"] || ($protobuf.roots["default"] = {});

export const messenger = $root.messenger = (() => {

    /**
     * Namespace messenger.
     * @exports messenger
     * @namespace
     */
    const messenger = {};

    messenger.SecureEnvelope = (function() {

        /**
         * Properties of a SecureEnvelope.
         * @typedef {Object} messenger.SecureEnvelope.$Properties
         * @property {Uint8Array|null} [nonce] SecureEnvelope nonce
         * @property {Uint8Array|null} [ciphertext] SecureEnvelope ciphertext
         * @property {string|null} [senderId] SecureEnvelope senderId
         * @property {string|null} [recipientId] SecureEnvelope recipientId
         * @property {Array.<Uint8Array>} [$unknowns] Unknown fields preserved while decoding
         */

        /**
         * Properties of a SecureEnvelope.
         * @memberof messenger
         * @interface ISecureEnvelope
         * @augments messenger.SecureEnvelope.$Properties
         * @deprecated Use messenger.SecureEnvelope.$Properties instead.
         */

        /**
         * Shape of a SecureEnvelope.
         * @typedef {messenger.SecureEnvelope.$Properties} messenger.SecureEnvelope.$Shape
         */

        /**
         * Constructs a new SecureEnvelope.
         * @memberof messenger
         * @classdesc Represents a SecureEnvelope.
         * @constructor
         * @param {messenger.SecureEnvelope.$Properties=} [properties] Properties to set
         * @property {Array.<Uint8Array>} [$unknowns] Unknown fields preserved while decoding
         */
        const SecureEnvelope = function (properties) {
            if (properties)
                for (let keys = $Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null && keys[i] !== "__proto__")
                        this[keys[i]] = properties[keys[i]];
        };

        /**
         * SecureEnvelope nonce.
         * @member {Uint8Array} nonce
         * @memberof messenger.SecureEnvelope
         * @instance
         */
        SecureEnvelope.prototype.nonce = $util.newBuffer([]);

        /**
         * SecureEnvelope ciphertext.
         * @member {Uint8Array} ciphertext
         * @memberof messenger.SecureEnvelope
         * @instance
         */
        SecureEnvelope.prototype.ciphertext = $util.newBuffer([]);

        /**
         * SecureEnvelope senderId.
         * @member {string} senderId
         * @memberof messenger.SecureEnvelope
         * @instance
         */
        SecureEnvelope.prototype.senderId = "";

        /**
         * SecureEnvelope recipientId.
         * @member {string} recipientId
         * @memberof messenger.SecureEnvelope
         * @instance
         */
        SecureEnvelope.prototype.recipientId = "";

        /**
         * Creates a new SecureEnvelope instance using the specified properties.
         * @function create
         * @memberof messenger.SecureEnvelope
         * @static
         * @param {messenger.SecureEnvelope.$Properties=} [properties] Properties to set
         * @returns {messenger.SecureEnvelope} SecureEnvelope instance
         * @type {{
         *   (properties: messenger.SecureEnvelope.$Shape): messenger.SecureEnvelope & messenger.SecureEnvelope.$Shape;
         *   (properties?: messenger.SecureEnvelope.$Properties): messenger.SecureEnvelope;
         * }}
         */
        SecureEnvelope.create = function(properties) {
            return new SecureEnvelope(properties);
        };

        /**
         * Encodes the specified SecureEnvelope message. Does not implicitly {@link messenger.SecureEnvelope.verify|verify} messages.
         * @function encode
         * @memberof messenger.SecureEnvelope
         * @static
         * @param {messenger.SecureEnvelope.$Properties} message SecureEnvelope message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        SecureEnvelope.encode = function (message, writer, _depth) {
            if (!writer)
                writer = $Writer.create();
            if (_depth === $undefined)
                _depth = 0;
            if (_depth > $util.recursionLimit)
                throw $Error("max depth exceeded");
            if (message.nonce != null && $Object.hasOwnProperty.call(message, "nonce"))
                writer.uint32(/* id 1, wireType 2 =*/10).bytes(message.nonce);
            if (message.ciphertext != null && $Object.hasOwnProperty.call(message, "ciphertext"))
                writer.uint32(/* id 2, wireType 2 =*/18).bytes(message.ciphertext);
            if (message.senderId != null && $Object.hasOwnProperty.call(message, "senderId"))
                writer.uint32(/* id 3, wireType 2 =*/26).string(message.senderId);
            if (message.recipientId != null && $Object.hasOwnProperty.call(message, "recipientId"))
                writer.uint32(/* id 4, wireType 2 =*/34).string(message.recipientId);
            if (message.$unknowns != null && $Object.hasOwnProperty.call(message, "$unknowns"))
                for (let i = 0; i < message.$unknowns.length; ++i)
                    writer.raw(message.$unknowns[i]);
            return writer;
        };

        /**
         * Encodes the specified SecureEnvelope message, length delimited. Does not implicitly {@link messenger.SecureEnvelope.verify|verify} messages.
         * @function encodeDelimited
         * @memberof messenger.SecureEnvelope
         * @static
         * @param {messenger.SecureEnvelope.$Properties} message SecureEnvelope message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        SecureEnvelope.encodeDelimited = function(message, writer) {
            return this.encode(message, writer && writer.len ? writer.fork() : writer).ldelim();
        };

        /**
         * Decodes a SecureEnvelope message from the specified reader or buffer.
         * @function decode
         * @memberof messenger.SecureEnvelope
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {messenger.SecureEnvelope & messenger.SecureEnvelope.$Shape} SecureEnvelope
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        SecureEnvelope.decode = function (reader, length, _end, _depth, _target) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            if (_depth === $undefined)
                _depth = 0;
            if (_depth > $Reader.recursionLimit)
                throw $Error("max depth exceeded");
            let end = length === $undefined ? reader.len : reader.pos + length, message = _target || new $root.messenger.SecureEnvelope(), value;
            while (reader.pos < end) {
                let start = reader.pos;
                let tag = reader.tag();
                if (tag === _end) {
                    _end = $undefined;
                    break;
                }
                let wireType = tag & 7;
                switch (tag >>>= 3) {
                case 1: {
                        if (wireType !== 2)
                            break;
                        if ((value = reader.bytes()).length)
                            message.nonce = value;
                        else
                            delete message.nonce;
                        continue;
                    }
                case 2: {
                        if (wireType !== 2)
                            break;
                        if ((value = reader.bytes()).length)
                            message.ciphertext = value;
                        else
                            delete message.ciphertext;
                        continue;
                    }
                case 3: {
                        if (wireType !== 2)
                            break;
                        if ((value = reader.string()).length)
                            message.senderId = value;
                        else
                            delete message.senderId;
                        continue;
                    }
                case 4: {
                        if (wireType !== 2)
                            break;
                        if ((value = reader.string()).length)
                            message.recipientId = value;
                        else
                            delete message.recipientId;
                        continue;
                    }
                }
                reader.skipType(wireType, _depth, tag);
                if (!reader.discardUnknown) {
                    $util.makeProp(message, "$unknowns", false);
                    (message.$unknowns || (message.$unknowns = [])).push(reader.raw(start, reader.pos));
                }
            }
            if (_end !== $undefined)
                throw $Error("missing end group");
            return message;
        };

        /**
         * Decodes a SecureEnvelope message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof messenger.SecureEnvelope
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {messenger.SecureEnvelope & messenger.SecureEnvelope.$Shape} SecureEnvelope
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        SecureEnvelope.decodeDelimited = function(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a SecureEnvelope message.
         * @function verify
         * @memberof messenger.SecureEnvelope
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        SecureEnvelope.verify = function (message, _depth) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (_depth === $undefined)
                _depth = 0;
            if (_depth > $util.recursionLimit)
                return "max depth exceeded";
            if (message.nonce != null && $Object.hasOwnProperty.call(message, "nonce"))
                if (!(message.nonce && typeof message.nonce.length === "number" || $util.isString(message.nonce)))
                    return "nonce: buffer expected";
            if (message.ciphertext != null && $Object.hasOwnProperty.call(message, "ciphertext"))
                if (!(message.ciphertext && typeof message.ciphertext.length === "number" || $util.isString(message.ciphertext)))
                    return "ciphertext: buffer expected";
            if (message.senderId != null && $Object.hasOwnProperty.call(message, "senderId"))
                if (!$util.isString(message.senderId))
                    return "senderId: string expected";
            if (message.recipientId != null && $Object.hasOwnProperty.call(message, "recipientId"))
                if (!$util.isString(message.recipientId))
                    return "recipientId: string expected";
            return null;
        };

        /**
         * Creates a SecureEnvelope message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof messenger.SecureEnvelope
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {messenger.SecureEnvelope} SecureEnvelope
         */
        SecureEnvelope.fromObject = function (object, _depth) {
            if (object instanceof $root.messenger.SecureEnvelope)
                return object;
            if (!$util.isObject(object))
                throw $TypeError(".messenger.SecureEnvelope: object expected");
            if (_depth === $undefined)
                _depth = 0;
            if (_depth > $util.recursionLimit)
                throw $Error("max depth exceeded");
            let message = new $root.messenger.SecureEnvelope();
            if (object.nonce != null)
                if (object.nonce.length)
                    if (typeof object.nonce === "string")
                        $util.base64.decode(object.nonce, message.nonce = $util.newBuffer($util.base64.length(object.nonce)), 0);
                    else if (object.nonce.length >= 0)
                        message.nonce = object.nonce;
            if (object.ciphertext != null)
                if (object.ciphertext.length)
                    if (typeof object.ciphertext === "string")
                        $util.base64.decode(object.ciphertext, message.ciphertext = $util.newBuffer($util.base64.length(object.ciphertext)), 0);
                    else if (object.ciphertext.length >= 0)
                        message.ciphertext = object.ciphertext;
            if (object.senderId != null)
                if (typeof object.senderId !== "string" || object.senderId.length)
                    message.senderId = $String(object.senderId);
            if (object.recipientId != null)
                if (typeof object.recipientId !== "string" || object.recipientId.length)
                    message.recipientId = $String(object.recipientId);
            return message;
        };

        /**
         * Creates a plain object from a SecureEnvelope message. Also converts values to other types if specified.
         * @function toObject
         * @memberof messenger.SecureEnvelope
         * @static
         * @param {messenger.SecureEnvelope} message SecureEnvelope
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        SecureEnvelope.toObject = function (message, options, _depth) {
            if (!options)
                options = {};
            if (_depth === $undefined)
                _depth = 0;
            if (_depth > $util.recursionLimit)
                throw $Error("max depth exceeded");
            let object = {};
            if (options.defaults) {
                if (options.bytes === $String)
                    object.nonce = "";
                else {
                    object.nonce = [];
                    if (options.bytes !== $Array)
                        object.nonce = $util.newBuffer(object.nonce);
                }
                if (options.bytes === $String)
                    object.ciphertext = "";
                else {
                    object.ciphertext = [];
                    if (options.bytes !== $Array)
                        object.ciphertext = $util.newBuffer(object.ciphertext);
                }
                object.senderId = "";
                object.recipientId = "";
            }
            if (message.nonce != null && $Object.hasOwnProperty.call(message, "nonce"))
                object.nonce = options.bytes === $String ? $util.base64.encode(message.nonce, 0, message.nonce.length) : options.bytes === $Array ? $Array.prototype.slice.call(message.nonce) : message.nonce;
            if (message.ciphertext != null && $Object.hasOwnProperty.call(message, "ciphertext"))
                object.ciphertext = options.bytes === $String ? $util.base64.encode(message.ciphertext, 0, message.ciphertext.length) : options.bytes === $Array ? $Array.prototype.slice.call(message.ciphertext) : message.ciphertext;
            if (message.senderId != null && $Object.hasOwnProperty.call(message, "senderId"))
                object.senderId = message.senderId;
            if (message.recipientId != null && $Object.hasOwnProperty.call(message, "recipientId"))
                object.recipientId = message.recipientId;
            return object;
        };

        /**
         * Converts this SecureEnvelope to JSON.
         * @function toJSON
         * @memberof messenger.SecureEnvelope
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        SecureEnvelope.prototype.toJSON = function() {
            return SecureEnvelope.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the type url for SecureEnvelope
         * @function getTypeUrl
         * @memberof messenger.SecureEnvelope
         * @static
         * @param {string} [prefix] Custom type url prefix, defaults to `"type.googleapis.com"`
         * @returns {string} The type url
         */
        SecureEnvelope.getTypeUrl = function(prefix) {
            if (prefix === $undefined)
                prefix = "type.googleapis.com";
            return prefix + "/messenger.SecureEnvelope";
        };

        return SecureEnvelope;
    })();

    return messenger;
})();

export {
  $root as default
};
